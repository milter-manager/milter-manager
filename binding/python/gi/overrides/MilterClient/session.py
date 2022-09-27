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

import gi
from gi.repository import GLib

import milter.core
StepFlags = milter.core.StepFlags
MilterClient = gi.module.get_introspection_module("MilterClient")
ClientContextState = MilterClient.ClientContextState

class Session(object):
    def __init__(self, context):
        self._context = context
        self.reset()

    def negotiate(self, option, macros_requests):
        events = [
            "connect",
            "helo",
            "envelope_from",
            "envelope_recipient",
            "data",
            "header",
            "end_of_header",
            "body",
            "unknown",
        ]
        for event in events:
            if not hasattr(self, event):
                continue
            if event == "header":
                step_name = "headers"
            else:
                step_name = event
            step = getattr(milter.core.StepFlags,
                           f"NO_{step_name.upper()}")
            option.remove_step(step)
        if self._need_header_value_with_leading_space():
            option.remove_step(StepFlags.HEADER_VALUE_WITH_LEADING_SPACE)
        option.remove_step(StepFlags.NO_REPLY_MASK)
        self._continue()

    def reset(self):
        pass

    def on_error(self, event, exception):
        pass

    def _need_header_value_with_leading_space(self):
        return False

    def _accept(self):
        self._context.status = "accept"
        self.reset()

    def _discard(self):
        self._context.status = "discard"
        self.reset()

    def _continue(self):
        self._context.status = "continue"

    def _delay_response(self):
        self._context.status = "progress"

    def _reject(self, code=None, extended_code=None, reason=None):
        if code or extended_code or reason:
            code = code or 550
            extended_code = extended_code or "5.7.1"
            reason = reason or "Command rejected"
            self._context.set_reply(code, extended_code, reason)
        else:
            self._context.status = "reject"
        if not self._context.state == ClientContextState.ENVELOPE_RECIPIENT:
            self.reset()

    def _temporary_failure(self, code=None, extended_code=None, reason=None):
        if code or extended_code or reason:
            code = code or 451
            extended_code = extended_code or "4.7.1"
            reason = reason or "Service unavailable - try again later"
            self._context.set_reply(code, extended_code, reason)
        else:
            self._context.status = "temporary_failure"
        if not self._context.state == ClientContextState.ENVELOPE_RECIPIENT:
            self.reset()

    def _quarantine(self, reason):
        success = self._context.quarantine(reason)
        if success:
            self._accept()
        return success

    def _add_header(self, name, value):
        self._context.add_header(name, value)

    def _insert_header(self, index, name, value):
        self._context.insert_header(index, name, value)

    def _change_header(self, name, index, value):
        self._context.change_header(name, index, value)

    def _delete_header(self, name, index):
        self._context.delete_header(name, index)

    def _replace_body(self, body):
        if not isinstance(body, GLib.Bytes):
            if isinstance(body, str):
                body = body.encode("utf-8")
            body = GLib.Bytes.new(body)
        self._context.replace_body(body)

    def _change_from(self, envelope_from):
        self._context.change_from(envelope_from)

    def _add_recipient(self, recipient, parameters=None):
        self._context.add_recipient(recipient, parameters)

    def _delete_recipient(self, recipient):
        self._context.delete_recipient(recipient)

    def _watch_child(self, pid, callback, priority=GLib.PRIORITY_DEFAULT):
        return self._context.event_loop.watch_child(priority, pid, callback)

    def _remove_source(self, source_id):
        return self._context.event_loop.remove(source_id)

    def __getitem__(self, name):
        return self._context[name]
