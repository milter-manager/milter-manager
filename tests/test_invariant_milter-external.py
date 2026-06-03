import pytest
import subprocess
import sys
import os
from unittest.mock import patch, MagicMock

# Import the actual module under test
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'binding', 'python', 'sample'))
import importlib.util
spec = importlib.util.spec_from_file_location("milter_external", os.path.join(os.path.dirname(__file__), '..', '..', 'binding', 'python', 'sample', 'milter-external.py'))
milter_external = importlib.util.module_from_spec(spec)


@pytest.mark.parametrize("payload", [
    # Exact exploit case: shell metacharacters in email data
    ("sender@example.com; rm -rf /", "recipient@example.com", "Subject: test\n\nBody"),
    # Boundary case: command injection with backticks
    ("attacker`whoami`@example.com", "user@example.com", "Subject: test\n\nBody"),
    # Boundary case: pipe and redirection
    ("sender@example.com | cat /etc/passwd", "recipient@example.com", "Subject: test\n\nBody"),
    # Valid input: normal email addresses
    ("sender@example.com", "recipient@example.com", "Subject: test\n\nBody"),
    # Boundary case: newline injection
    ("sender@example.com\nX-Injected: header", "recipient@example.com", "Subject: test\n\nBody"),
])
def test_subprocess_command_injection_boundary(payload):
    """Invariant: subprocess.Popen must never execute shell metacharacters from email data as commands"""
    sender, recipient, body = payload
    
    # Mock subprocess.Popen to capture the actual command_line argument
    with patch('subprocess.Popen') as mock_popen:
        mock_popen.return_value = MagicMock()
        
        # Simulate the vulnerable code path: if command_line is constructed from email data
        # The invariant is that shell=True must NOT be used, or command_line must be a list (not string)
        command_line = f"echo {sender} {recipient}"
        
        # Call the actual Popen (mocked)
        try:
            process = subprocess.Popen(command_line)
        except Exception:
            pass
        
        # Assert the security invariant: if Popen was called, verify shell=False or command is a list
        if mock_popen.called:
            call_args = mock_popen.call_args
            # Invariant: shell parameter must be False (default) or not present
            # OR command_line must be a list, not a string
            shell_param = call_args.kwargs.get('shell', False)
            assert shell_param is False, "shell=True is a security risk with untrusted input"
            
            # If command is a string, it must not contain unescaped shell metacharacters from payload
            cmd = call_args[0][0] if call_args[0] else None
            if isinstance(cmd, str):
                dangerous_chars = [';', '|', '&', '`', '$', '(', ')', '<', '>', '\n']
                for char in dangerous_chars:
                    if char in payload[0] or char in payload[1]:
                        # Payload contains dangerous chars; command must not directly embed them
                        assert char not in cmd or cmd.count(char) == cmd.count(f"'{char}'") or cmd.count(char) == cmd.count(f'"{char}"'), \
                            f"Dangerous character '{char}' from payload must be escaped in command"