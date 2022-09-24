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
import sys

import gi._ossighelper

import milter.core
from .client import Client

class CommandLine(object):
    def __init__(self, **kwargs):
        self.parser = argparse.ArgumentParser(**kwargs)
        self._setup_arguments()

    def parse(self, argv=None):
        args = self.parser.parse_args(argv)
        if not hasattr(args, "config"):
            args.config = configparser.ConfigParser()
        return args

    @contextlib.contextmanager
    def run(self, argv=None):
        args = self.parse(argv)
        client = Client()
        def on_error(_client, error):
            milter.core.Logger.error(f"[client][error] {type(error)}: {error}")
        self._setup_client(client, args)
        client.connect("error", on_error)
        client.event_loop = client.create_event_loop(True)
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

            def set_config(key, section, option, getter=config.get):
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

    def _setup_basic_arguments(self):
        basic = self.parser.add_argument_group("Basic", "Basic options")
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
                           help="Set milter environment as ENVIRONMENT",
                           metavar="ENVIRONMENT")

    def _setup_milter_arguments(self):
        milter = self.parser.add_argument_group("milter", "milter options")
        milter.add_argument("-s", "--connection-spec",
                            default="inet:20025",
                            dest="connection_spec",
                            help="Specify connection spec as SPEC",
                            metavar="SPEC")
        milter.add_argument("--daemon",
                            action=argparse.BooleanOptionalAction,
                            dest="daemon",
                            help="Run as a daemon process")
        milter.add_argument("--pid-file",
                            dest="pid_file",
                            metavar="FILE",
                            help="Write process ID to FILE")

        # TODO: See Milter::Client::CommandLine#setup_milter_options

    def _setup_logger_arguments(self):
        pass

        # TODO: See Milter::Client::CommandLine#setup_logger_options

    def _setup_client(self, client, args):
        client.set_connection_spec(args.connection_spec)
        if args.pid_file:
            client.set_pid_file(args.pid_file)
