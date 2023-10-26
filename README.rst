SEFT
====

.. image:: https://www.github.com/RyugaXhypeR/seft/actions/workflows/c-cpp.yml/badge.svg
   :alt: Build Status
   :target: https://github.com/RyugaXhypeR/seft/actions

SEFT (SSH Encrypted File Transfer) is a cli tool for interacting with SFTP servers.


Requirements
------------

Libraries
^^^^^^^^^
    * `libssh <https://www.libssh.org>`_
    * `argp <https://www.gnu.org/software/libc/manual/html_node/Argp.html>`_

Build
^^^^^
    * `autoconf <https://www.gnu.org/software/autoconf/>`_


Installation
------------

On Unix, Linux and macOS::

    autoreconf --install
    ./configure
    make
    make install

This will install seft as ``seft``.


Usage
-----

After installation, the ``seft`` command will be available.

You can use the ``--help`` option to get a list of available commands::

    seft --help

Connecting to a server::

    seft connect --subsystem <subsystem> --port <port>


License
-------

This project is licensed under the MIT License
