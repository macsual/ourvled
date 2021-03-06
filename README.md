# ourvled

This daemon is a HTTP client that currently does not completely conform to
spec. It communicates with a [Moodle](https://moodle.org/) web services server
using the Moodle Mobile web service which is provided via a REST API.
Specifically, it communicates with [OurVLE](https://ourvle.mona.uwi.edu/), which
is a Moodle instance deployed by the [University of The West Indies](https://www.mona.uwi.edu/).
The program is optimised with respect to OurVLE in that:

* OurVLE does not support IPv6, so network code is written to support IPv4 only.

* OurVLE does not configure Moodle web services to be offered over HTTPS, so
protocol code is written to only support regular HTTP.

The program communicates over HTTP 1.1, in order to take advantage of
keep-alive connections. REST API responses are requested to be formatted in
[JSON](https://www.json.org/), due to its ubiquity, easiness to parse and its
similarity to data representation in code. Additionally, the source code of the
[nginx web server](https://www.nginx.com/) is used to help guide in the
development of highly optimised software.


In the future, perhaps this software could be extended to support any instance
of Moodle, which would involve adding HTTPS and IPv6 support, in addition to
extending other code tailored specifically to OurVLE.
