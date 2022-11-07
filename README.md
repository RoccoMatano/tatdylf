# ![](https://raw.githubusercontent.com/RoccoMatano/tatdylf/master/doc/r2d2.png) tatdylf

[![License - MIT](https://img.shields.io/badge/license-MIT-green)](https://spdx.org/licenses/MIT.html)

-----

tatdylf is a stripped-down Windows DHCP server whose sole task is to provide
[Basler](https://www.baslerweb.com/) GigE-Vision cameras with IP addresses
from a class C private network (192.168.x.x). The limitation to this operational
objective allows many simplifications that would be inadmissible in the general
case. E.g. the processing of DHCP options and memory reservation for those can
be reduced to just handling a handful of those.
