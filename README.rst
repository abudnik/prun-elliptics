Overview
--------
Prun job serialization library.

Building
--------

Build requirements:

- cmake 2.6 (or higher)
- GCC 4.6 (or higher) or Clang
- boost 1.46 (or higher)
- elliptics-client-dev

Building debian packages::

> git clone https://github.com/abudnik/prun-elliptics.git
> cd prun-elliptics
> debuild -sa
> ls ../prun-elliptics*.deb

Building RPMs::

> git clone https://github.com/abudnik/prun-elliptics.git
> mv prun-elliptics prun-elliptics-0.1  # add '-version' postfix
> mkdir -p rpmbuild/SOURCES
> tar cjf rpmbuild/SOURCES/prun-elliptics-0.1.tar.bz2 prun-elliptics-0.1
> rpmbuild -ba prun-elliptics-0.1/prun-elliptics-bf.spec
> ls rpmbuild/RPMS/*/*.rpm

Building runtime from sources::

> git clone https://github.com/abudnik/prun-elliptics.git
> cd prun-elliptics
> cmake && make

Configuration
-------------

Example of prun-elliptics config file: prun/conf/history-elliptics.cfg

Config is represented in the JSON formatted file with following properties:

| ``log`` - path to a log file.
| ``log_level`` - must be one of the following: error, warning, info, notice, debug.
| ``remotes`` - array of elliptics remote server nodes.
| ``groups`` - array of group ids (i.e. replica ids).

For more detailed information see http://doc.reverbrain.com/elliptics:configuration
