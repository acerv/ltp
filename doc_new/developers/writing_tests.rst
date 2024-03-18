.. SPDX-License-Identifier: GPL-2.0-or-later

Writing tests
=============

Testing builds with GitHub Actions
----------------------------------

Master branch is tested in `GitHub Actions <https://github.com/linux-test-project/ltp/actions>`_
to ensure LTP builds in various distributions, including old, current and
bleeding edge. ``gcc`` and ``clang`` toolchains are also tested for various
architectures using cross-compilation. For a full list of tested distros, please
check ``.github/workflows/ci.yml``.

.. note::

      Passing the GitHub Actions CI means that LTP compiles in a variety of
      different distributions on their **newest releases**.
      The CI also checks for code linting, running ``make check`` in the whole
      LTP project.
