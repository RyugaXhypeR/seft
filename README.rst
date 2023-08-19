SFTP CLI
========

.. image:: https://www.github.com/RyugaXhypeR/sftp-cli/.github/workflows/c-cpp.yml/badge.svg
    :target: https://github.com/RyugaXhypeR/sftp-cli/actions

A cli tool for interacting with SFTP servers.


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

This will install sftp-cli as ``sftp_cli``.


Usage
-----

After installation, the ``sftp_cli`` command will be available.

You can use the ``--help`` option to get a list of available commands::

    sftp_cli --help

Connecting to a server::

    sftp_cli --subsystem <subsystem> --port <port>


License
-------

This project is licensed under the MIT License
