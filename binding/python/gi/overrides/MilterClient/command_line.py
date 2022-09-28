# Copyright (C) 2022  Sutou Kouhei <kou@clear-code.com>
#
# This library is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this library.  If not, see <http://www.gnu.org/licenses/>.

import argparse
import configparser
import contextlib
import os
import resource
import sys
import syslog

import gi._ossighelper
import gi.module

import milter.core
from .client import Client

MilterClient = gi.module.get_introspection_module("MilterClient")

class CommandLine(object):
    AVAILABLE_EVENT_LOOP_BACKENDS = [
        enum.value_nick
        for enum
        in MilterClient.ClientEventLoopBackend.__enum_values__.values()
    ]

    AVAILABLE_FALLBACK_STATUSES = [
        "accept",
        "reject",
        "temporary-failure",
        "discard",
    ]

    def __init__(self, name=None, version=None, **kwargs):
        self.name = name or os.path.splitext(os.path.basename(sys.argv[0]))[0]
        self.version = version
        self.parser = argparse.ArgumentParser(**kwargs)
        self._setup_arguments()

    @contextlib.contextmanager
    def run(self, argv=None):
        args = self._parse(argv)
        self._apply_args(args)
        client = Client()
        def on_error(_client, error):
            milter.core.Logger.error(f"[client][error] {type(error)}: {error}")
        self._setup_client(client, args)
        client.connect("error", on_error)
        client.set_event_loop(client.create_event_loop(True))
        yield client, args
        client.listen()
        client.drop_privilege()
        if args.daemon:
            client.daemonize()
        with gi._ossighelper.register_sigint_fallback(lambda: client.shutdown()):
            with gi._ossighelper.wakeup_on_signal():
                client.run()

    def _setup_arguments(self):
        self._setup_basic_arguments()
        self._setup_milter_arguments()
        self._setup_logger_arguments()

    class ShowLibraryVersionAction(argparse.Action):
        def __call__(self, *args, **kwargs):
            print(milter.core.VERSION)
            sys.exit(0)

    class ConfigurationLoadAction(argparse.Action):
        def __call__(self, parser, namespace, path, option_string):
            namespace.config_path = path
            if not hasattr(namespace, "config"):
                namespace.config = configparser.ConfigParser()
            config = namespace.config
            with open(path) as input:
                config.read_file(input, path)

            def set_config(key, section, option,
                           getter=config.get,
                           available_values=None):
                if not section in config:
                    return
                current_value = getattr(namespace, key)
                setattr(namespace,
                        key,
                        getter(section, key, fallback=current_value))

            set_config("environment", "basic", "environment")
            set_config("connection_spec", "milter", "connection_spec")
            set_config("daemon", "milter", "daemon", config.getboolean)
            set_config("pid_file", "milter", "pid_file")
            set_config("fallback_status",
                       "milter",
                       "fallback_status",
                       available_values=CommandLine.AVAILABLE_FALLBACK_STATUSES)
            set_config("effective_user", "milter", "user")
            set_config("effective_group", "milter", "group")
            set_config("unix_socket_group", "milter", "unix_socket_group")
            set_config("unix_socket_mode", "milter", "unix_socket_mode")
            set_config("max_file_descriptors", "milter", "max_file_descriptors")
            set_config("event_loop_backend",
                       "milter",
                       "event_loop_backend",
                       available_values=CommandLine.AVAILABLE_EVENT_LOOP_BACKENDS)
            set_config("log_level", "log", "level")
            set_config("log_path", "log", "path")
            set_config("use_syslog", "log", "use_syslog", config.getboolean)
            set_config("syslog_facility", "log", "syslog_facility")

    def _setup_basic_arguments(self):
        basic = self.parser.add_argument_group("Basic", "Basic options")
        if self.version:
            basic.add_argument("--version",
                               action="version",
                               help="Show version",
                               version=f"%(prog)s {self.version}")
        basic.add_argument("--library-version",
                           action=self.ShowLibraryVersionAction,
                           help="Show milter library version",
                           nargs=0)
        basic.add_argument("-c", "--configuration",
                           action=self.ConfigurationLoadAction,
                           help="Load configuration from FILE",
                           metavar="FILE")
        basic.add_argument("-e", "--environment",
                           default=os.environ.get("MILTER_ENV", "development"),
                           dest="environment",
                           help="Set milter environment as ENVIRONMENT.\n" +
                           "(default: %(default)s)",
                           metavar="ENVIRONMENT")

    def _setup_milter_arguments(self):
        milter = self.parser.add_argument_group("milter", "milter options")
        milter.add_argument("--name",
                            default=self.name,
                            dest="name",
                            help="Specify name as NAME.\n" +
                            "(default: %(default)s)",
                            metavar="NAME")
        milter.add_argument("-s", "--connection-spec",
                            default="inet:20025",
                            dest="connection_spec",
                            help="Specify connection spec as SPEC.\n" +
                            "(default: %(default)s)",
                            metavar="SPEC")
        if hasattr(argparse, "BooleanOptionalAction"):
            milter.add_argument("--daemon",
                                action=argparse.BooleanOptionalAction,
                                dest="daemon",
                                help="Run as a daemon process")
        milter.add_argument("--pid-file",
                            dest="pid_file",
                            help="Write process ID to FILE",
                            metavar="FILE")
        milter.add_argument("--fallback-status",
                            choices=self.AVAILABLE_FALLBACK_STATUSES,
                            default="accept",
                            dest="fallback_status",
                            help="Use STATUS as fallback status.\n" +
                            "(available values: %(choices)s)\n" +
                            "(default: %(default)s)",
                            metavar="STATUS")
        milter.add_argument("--user",
                            dest="effective_user",
                            help="Run as USER's process (need root privilege)",
                            metavar="USER")
        milter.add_argument("--group",
                            dest="effective_group",
                            help="Run as GROUP's process (need root privilege)",
                            metavar="GROUP")
        milter.add_argument("--unix-socket-group",
                            dest="unix_socket_group",
                            help="Change UNIX domain socket group to GROUP",
                            metavar="GROUP")
        milter.add_argument("--unix-socket-mode",
                            dest="unix_socket_mode",
                            help="Change UNIX domain socket mode to MODE",
                            metavar="MODE")
        milter.add_argument("--max-file-descriptors",
                            dest="max_file_descriptors",
                            help="Change maximum number of file descriptors to N",
                            metavar="N",
                            type=int)
        milter.add_argument("--event-loop-backend",
                            choices=self.AVAILABLE_EVENT_LOOP_BACKENDS,
                            default="default",
                            dest="event_loop_backend",
                            help="Use BACKEND as event loop backend.\n" +
                            "(available values: %(choices)s)\n" +
                            "(default: %(default)s)",
                            metavar="BACKEND")

        # TODO: See Milter::Client::CommandLine#setup_milter_options

    def _setup_logger_arguments(self):
        logger = self.parser.add_argument_group("Logger", "Logger options")
        level_names = [
            value.first_value_nick
            for value
            in milter.core.LogLevelFlags.__flags_values__.values()
        ]
        level_names += ["all"]
        logger.add_argument("--log-level",
                            default="default",
                            dest="log_level",
                            help="Specify log level as LEVEL.\n" +
                            "You can set levels by LEVEL1|LEVEL2|...." +
                            "You can add one or more levels " +
                            "to the default levels by +LEVEL1|+LEVEL2." +
                            "You can remove one or more levels " +
                            "from the default levels by -LEVEL1|-LEVEL2." +
                            f"(available levels: {', '.join(level_names)})\n" +
                            "(default: %(default)s)",
                            metavar="LEVEL")
        logger.add_argument("--log-path",
                            default="-",
                            dest="log_path",
                            help="Specify log output path as PATH.\n" +
                            "If PATH is '-', the standard output is used.\n" +
                            "(default: %(default)s)",
                            metavar="PATH")
        if hasattr(argparse, "BooleanOptionalAction"):
            logger.add_argument("--syslog",
                                default=False,
                                action=argparse.BooleanOptionalAction,
                                dest="use_syslog",
                                help="Use syslog")
        no_facility_names = [
            "LOG_EMERG",
            "LOG_ALERT",
            "LOG_CRIT",
            "LOG_ERR",
            "LOG_WARNING",
            "LOG_NOTICE",
            "LOG_INFO",
            "LOG_DEBUG",
            "LOG_PID",
            "LOG_CONS",
            "LOG_NDELAY",
            "LOG_ODELAY",
            "LOG_NOWAIT",
            "LOG_PERROR",
        ]
        syslog_facilities = [
            key.replace("LOG_", "", 1).lower()
            for key
            in syslog.__dict__
            if key.startswith("LOG_") and not key in no_facility_names
        ]
        logger.add_argument("--syslog-facility",
                            choices=syslog_facilities,
                            default="mail",
                            dest="syslog_facility",
                            help="Use FACILITY as syslog facility.\n" +
                            "(available values: %(choices)s).\n" +
                            "(default: %(default)s)",
                            metavar="FACILITY")
        logger.add_argument("--verbose",
                            action="store_const",
                            const="all",
                            dest="log_level",
                            help="Show messages verbosely.\n" +
                            "Alias of --log-level=all.")

    def _parse(self, argv=None):
        args = self.parser.parse_args(argv)
        if not hasattr(args, "config"):
            args.config = configparser.ConfigParser()
        return args

    def _apply_args(self, args):
        milter.core.Logger.get_default().target_level = args.log_level
        milter.core.Logger.get_default().path = args.log_path

    def _setup_client(self, client, args):
        client.set_connection_spec(args.connection_spec)
        if args.pid_file:
            client.set_pid_file(args.pid_file)
        if args.fallback_status:
            client.fallback_status = args.fallback_status
        if args.use_syslog:
            client.start_syslog(args.name, args.syslog_facility)
        if args.effective_user:
            client.set_effective_user(args.effective_user)
        if args.effective_group:
            client.set_effective_user(args.effective_group)
        if args.unix_socket_group:
            client.set_unix_socket_group(args.unix_socket_group)
        mode = args.unix_socket_mode
        if mode:
            if mode.startswith("0"):
                mode = int(mode, 8)
            else:
                mode = int(mode)
            client.set_unix_socket_mode(mode)
        max_file_descriptors = args.max_file_descriptors
        if max_file_descriptors:
            max_file_descriptors = int(max_file_descriptors)
            if max_file_descriptors > 0:
                resource.setrlimit(resource.RLIMIT_NOFILE,
                                   (max_file_descriptors, max_file_descriptors))
        if args.event_loop_backend:
            client.set_event_loop_backend(
                getattr(MilterClient.ClientEventLoopBackend,
                        args.event_loop_backend.upper()))
