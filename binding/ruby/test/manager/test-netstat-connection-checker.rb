# Copyright (C) 2010-2011  Kouhei Sutou <kou@clear-code.com>
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

class TestNetstatConnectionChecker < Test::Unit::TestCase
  def setup
    @checker = Milter::Manager::NetstatConnectionChecker.new
  end

  def test_parse_netstat_result_linux
    def @checker.netstat
      <<-EON
Active Internet connections (w/o servers)
Proto Recv-Q Send-Q Local Address           Foreign Address         State      
tcp        0      0 192.168.3.2:55881       192.168.2.9:80          ESTABLISHED
tcp        0      0 192.168.3.2:55877       192.168.2.9:80          ESTABLISHED
tcp        1      0 192.168.3.2:34927       192.168.1.215:80        CLOSE_WAIT 
tcp        0      0 192.168.3.2:55880       192.168.2.9:80          ESTABLISHED
tcp        1      0 192.168.3.2:45946       192.168.4.195:80        CLOSE_WAIT 
tcp        1      0 192.168.3.2:53952       192.168.232.13:80       CLOSE_WAIT 
tcp6       0      0 ::1:80                  ::1:55496               ESTABLISHED
tcp6       0      0 ::1:55496               ::1:80                  ESTABLISHED
udp6       0      0 ::1:33134               ::1:33134               ESTABLISHED
Active UNIX domain sockets (w/o servers)
Proto RefCnt Flags       Type       State         I-Node   Path
unix  2      [ ]         DGRAM                    2326     @/org/kernel/udev/udevd
unix  28     [ ]         DGRAM                    4994     /dev/log
unix  2      [ ]         DGRAM                    11487    @/org/freedesktop/hal/udev_event
EON
    end
    @checker.send(:update_database)
    assert_equal({
                   :tcp4 => {
                     "192.168.2.9:80" => info("tcp4",
                                              "192.168.3.2", "55880",
                                              "192.168.2.9", "80",
                                              "ESTABLISHED"),
                     "192.168.1.215:80" => info("tcp4",
                                                "192.168.3.2", "34927",
                                                "192.168.1.215", "80",
                                                "CLOSE_WAIT"),
                     "192.168.4.195:80" => info("tcp4",
                                                "192.168.3.2", "45946",
                                                "192.168.4.195", "80",
                                                "CLOSE_WAIT"),
                     "192.168.232.13:80" => info("tcp4",
                                                 "192.168.3.2", "53952",
                                                 "192.168.232.13", "80",
                                                 "CLOSE_WAIT"),
                   },
                   :tcp6 => {
                     "::1:80" => info("tcp6",
                                      "::1", "55496",
                                      "::1", "80",
                                      "ESTABLISHED"),
                     "::1:55496" => info("tcp6",
                                         "::1", "80",
                                         "::1", "55496",
                                         "ESTABLISHED"),
                   }
                 },
                 @checker.instance_variable_get(:@database))
  end

  def test_parse_netstat_result_freebsd
    def @checker.netstat
      <<-EON
Active Internet connections
Proto Recv-Q Send-Q  Local Address          Foreign Address        (state)
tcp4       0      0 192.168.28.10.22       192.168.40.236.62353   SYN_RCVD
tcp4       0   1154 192.168.28.10.443      192.168.67.236.53978   ESTABLISHED
tcp4       0      0 192.168.28.10.80       192.168.195.71.38083   TIME_WAIT
tcp4       0      0 192.168.28.10.80       192.168.30.148.36748   TIME_WAIT
tcp4       0      0 192.168.28.10.80       192.168.67.50.52795    ESTABLISHED
tcp4       0      0 192.168.28.10.22       192.168.7.13.54099     LAST_ACK
tcp4       0      0 127.0.0.1.11100        127.0.0.1.52289        ESTABLISHED
tcp4       0      0 192.168.28.10.25       192.168.226.190.33787  ESTABLISHED
tcp4       0      0 192.168.28.10.80       192.168.28.10.63207    TIME_WAIT
tcp6       0      0 fe80::215:f2ff:fedb:e04c.54765                   fe80::215:f2ff:fedb:e04c.62269                   FIN_WAIT_2
tcp6       0      0 fe80::215:f2ff:fedb:e04c.62269                   fe80::215:f2ff:fedb:e04c.54765                   CLOSE_WAIT
tcp4       0      0 127.0.0.1.59355        127.0.0.1.4949         TIME_WAIT
udp4       0      0 192.168.28.10.123      *.*                    
udp6       0      0 fe80:1::215:f2ff:fedb:e04c.123                *.*            
Active UNIX domain sockets
Address  Type   Recv-Q Send-Q    Inode     Conn     Refs  Nextref Addr
c605a498 stream      0      0        0 c6dd99d8        0        0 private/rewrite
c6dd99d8 stream      0      0        0 c605a498        0        0
c95e9738 stream      0      0        0 c9ad4c78        0        0 public/cleanup
c9ad4c78 stream      0      0        0 c95e9738        0        0
c5f667e0 stream      0      0        0 c64b8888        0        0 private/rewrite
EON
    end
    @checker.send(:update_database)
    assert_equal({
                   :tcp4 => {
                     "192.168.28.10:63207" => info("tcp4",
                                                   "192.168.28.10", "80",
                                                   "192.168.28.10", "63207",
                                                   "TIME_WAIT"),
                     "192.168.195.71:38083" => info("tcp4",
                                                    "192.168.28.10", "80",
                                                    "192.168.195.71", "38083",
                                                    "TIME_WAIT"),
                     "192.168.226.190:33787" => info("tcp4",
                                                     "192.168.28.10", "25",
                                                     "192.168.226.190", "33787",
                                                     "ESTABLISHED"),
                     "192.168.67.236:53978" => info("tcp4",
                                                    "192.168.28.10", "443",
                                                    "192.168.67.236", "53978",
                                                    "ESTABLISHED"),
                     "192.168.30.148:36748" => info("tcp4",
                                                    "192.168.28.10", "80",
                                                    "192.168.30.148", "36748",
                                                    "TIME_WAIT"),
                     "192.168.67.50:52795" => info("tcp4",
                                                   "192.168.28.10", "80",
                                                   "192.168.67.50", "52795",
                                                   "ESTABLISHED"),
                     "127.0.0.1:4949" => info("tcp4",
                                              "127.0.0.1", "59355",
                                              "127.0.0.1", "4949",
                                              "TIME_WAIT"),
                     "127.0.0.1:52289" => info("tcp4",
                                               "127.0.0.1", "11100",
                                               "127.0.0.1", "52289",
                                               "ESTABLISHED"),
                     "192.168.7.13:54099" => info("tcp4",
                                                  "192.168.28.10", "22",
                                                  "192.168.7.13", "54099",
                                                  "LAST_ACK"),
                     "192.168.40.236:62353" => info("tcp4",
                                                    "192.168.28.10", "22",
                                                    "192.168.40.236", "62353",
                                                    "SYN_RCVD"),
                   },
                   :tcp6 => {
                     "fe80::215:f2ff:fedb:e04c:54765" =>
                        info("tcp6",
                             "fe80::215:f2ff:fedb:e04c", "62269",
                             "fe80::215:f2ff:fedb:e04c", "54765",
                             "CLOSE_WAIT"),
                     "fe80::215:f2ff:fedb:e04c:62269" =>
                        info("tcp6",
                             "fe80::215:f2ff:fedb:e04c", "54765",
                             "fe80::215:f2ff:fedb:e04c", "62269",
                             "FIN_WAIT_2"),
                   }
                 },
                 @checker.instance_variable_get(:@database))
  end

  private
  def info(protocol,
           local_ip_address, local_port,
           foreign_ip_address, foregin_port,
           status)
    info_class = Milter::Manager::NetstatConnectionChecker::ConnectionInfo
    info_class.new(protocol,
                   local_ip_address, local_port,
                   foreign_ip_address, foregin_port,
                   status)
  end
end
