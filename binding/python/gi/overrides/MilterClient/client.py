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

import gi.module

import milter.core
from .session_context import SessionContext

MilterClient = gi.module.get_introspection_module("MilterClient")
Client = MilterClient.Client

def get_fallback_status(self):
    if hasattr(self, "_fallback_status"):
        return getattr(self, "_fallback_status")
    else:
        return "accept"
Client.fallback_status = property(get_fallback_status)

def set_fallback_status(self, status):
    self._fallback_status = status
Client.fallback_status = Client.fallback_status.setter(set_fallback_status)

def register(self, session_class, *init_arguments):
    def on_connection_established(client, context):
        client._setup_session(context, session_class, init_arguments)
    self.connect("connection-established", on_connection_established)
Client.register = register

start_syslog_raw = Client.start_syslog
def start_syslog(self, identify, facility=None):
    self.set_syslog_identify(identify)
    if facility:
        self.set_syslog_facility(facility)
    start_syslog_raw(self)
Client.start_syslog = start_syslog

def _setup_session(self, context, session_class, init_arguments):
    session_context = SessionContext(context)
    session_context.fallback_status = self.fallback_status
    session = session_class(session_context, *init_arguments)

    context.set_use_bytes(True)

    def create_on_event(event):
        def on_event(context, *args):
            try:
                event_callable = getattr(session, event)
                if event == "end_of_message":
                    event_callable()
                else:
                    if event == "body":
                        # GBytes -> byte
                        args = (args[0].get_data(), *args[1:])
                    event_callable(*args)
            except Exception as exception:
                milter.core.Logger.default.error(exception)
                try:
                    session.on_error(event, exception)
                except Exception as nested_exception:
                    milter.core.Logger.default.error(nested_exception)
                finally:
                    session_context.status = session_context.fallback_status
            status = session_context.status
            session_context.clear()
            return status
        return on_event
    events = [
        "negotiate",
        "connect",
        "helo",
        "envelope_from",
        "envelope_recipient",
        "data",
        "unknown",
        "header",
        "end_of_header",
        "body",
        "end_of_message",
        "finished",
    ]
    for event in events:
        if not hasattr(session, event):
            continue
        if event == "body" or event == "end_of_message":
            real_event = event + "_bytes"
        else:
            real_event = event
        context.connect(real_event, create_on_event(event))

    def on_abort(context, *args):
        if hasattr(session, "abort"):
            try:
                session.abort(*args)
            except Exception as error:
                milter.core.Logger.error(error)
                session_context.status = session_context.fallback_status

        try:
            session.reset()
        except Exception as error:
            milter.core.Logger.error(error)
            session_context.status = session_context.fallback_status

        status = session_context.status
        session_context.clear()
        return status
    context.connect("abort", on_abort)
Client._setup_session = _setup_session
