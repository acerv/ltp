.. SPDX-License-Identifier: GPL-2.0-or-later

Linux Test Project
==================

Linux Test Project is a joint project started by SGI, OSDL and Bull developed
and maintained by SUSE, Red Hat, Fujitsu, IBM, Cisco, Oracle and others. The
project goal is to deliver tests to the open source community that validate
reliability, robustness, and stability of the Linux Kernel.

The testing suites contain a collection of tools for testing the Linux kernel
and related features. Our goal is to improve the Linux kernel and system
libraries by bringing test automation.

.. warning::

   LTP tests shouldn't run in production systems. In particular,
   growfiles, doio, and iogen, stress the I/O capabilities of the systems and
   they are intended to find (or cause) problems.

Some references:

* `Source code <https://github.com/linux-test-project/ltp>`_
* `Releases <https://github.com/linux-test-project/ltp/releases>`_
* `Mailing List <http://lists.linux.it/listinfo/ltp>`_
* `Working patches (patchwork) <https://patchwork.ozlabs.org/project/ltp/list/>`_
* `Working patches (lore.kernel.org) <https://lore.kernel.org/ltp>`_
* `#ltp @ libera chat <https://libera.chat/>`_

.. toctree::
   :maxdepth: 3
   :hidden:
   :caption: For users

   users/quick_start
   users/setup_tests
   users/supported_systems

.. toctree::
   :maxdepth: 3
   :hidden:
   :caption: For developers

   developers/setup_mailinglist
   developers/writing_tests
   developers/test_case_tutorial
   developers/api_c_tests
   developers/api_shell_tests
   developers/api_network_tests
   developers/api_kvm_tests
   developers/ltp_library
   developers/build_system
   developers/debugging

.. toctree::
   :maxdepth: 3
   :hidden:
   :caption: For maintainers

   maintainers/patch_review
   maintainers/ltp_release_procedure

For users
---------

.. descriptions here are active

:doc:`users/quick_start`
   How to build and use LTP framework in few steps

:doc:`users/setup_tests`
   How to setup tests execution

:doc:`users/supported_systems`
   A list of supported technologies by the LTP framework

For developers
--------------

.. descriptions here are active

:doc:`developers/setup_mailinglist`
   How to configure git and to start sending patches via ``git send-email``

:doc:`developers/writing_tests`
   Starting guide on writing tests

:doc:`developers/test_case_tutorial`
   A tutorial showing how to write a test from scratch using C API

:doc:`developers/api_c_tests`
   Walk through the C API features

:doc:`developers/api_shell_tests`
   Walk through the Shell API features

:doc:`developers/api_network_tests`
   Walk through the network API features

:doc:`developers/api_kvm_tests`
   Walk through the KVM API features

:doc:`developers/ltp_library`
   Developing new features in the LTP library

:doc:`developers/build_system`
   Short introduction to the LTP build system

:doc:`developers/debugging`
   How to debug LTP tests

For maintainers
---------------

:doc:`maintainers/patch_review`
   Steps to follow when reviewing patches

:doc:`maintainers/ltp_release_procedure`
   Steps to follow for a new LTP release


Getting help
------------
To report a problem or suggest any feature, please write at ltp@lists.linux.it
