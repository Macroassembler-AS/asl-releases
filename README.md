The Macroassembler AS Releases
==============================

[The Macroassembler AS][asl], also known as "ASL," is a multi-platform
cross-assembler. Build platforms include a variety of Unix systems
(including Linux and MacOS X), Windows, OS/2 and DOS (both native and
with an extender). Target platforms cover a huge variety of 8- and
16-bit CPUs and microcontrollers.

The `upstream` branch of this repository contains the source code for
every publicly available [source release][src] of the C version. The
tools to do this and this documentation are on the `master` branch.

Pull requests (to improve the import system and its documentation) are
accepted for the `master` branch. The `upstream` branch containing the
vendor sources never contains patches, but there may be patch branches
derived from imported vendor commits on `upstream`.

### Branches in this Repo

- `master`: The import script and its documentation.
- `upstream`: Imported ASL source code for each release.
- `dev/cjs/current`: A recent version tested to build on Linux and
  assemble a small amount of 6502 and 6909 code. The build and test
  framework is in cjs's [8bitdev] repo.
- `dev/NAME/...`: Branches for development, testing, and patches for
  particular versions of ASL.


Building ASL
------------

The detailed instructions are with the ASL code itself in the `upstream`
branch. However, as a quick guide for Linux:

    #   Debian/Ubuntu package manager and package names: tweak for your distro.
    sudo apt-get install build-essential texlive texlive-lang-german
    git checkout upstream
    cp Makefile.def-samples/Makefile.def-x86_64-unknown-linux Makefile.def
    #   edit Makefile.def to set install path
    make -j8        # does not build docs
    make install    # optional, but needed for it to find libary include files

There is also a `Makefile.def` on the `master` branch that should work with
most Linux systems and will install ASL under `/opt/asl-$VER`. However, it
may or may not determine the version nubmer correctly as ASL has changed
how it defines version numbers over time.


Importing New ASL Releases
--------------------------

Copy the `download.sh` script to a location outside of the
repository in which you're going to do the import, change to the
`upstream` branch, and then run `download.sh`. This will find all new
[source releases][src] that have not yet been imported and allow you
to import them one by one, generating a new commit for every new
release.


Authors and Maintainers
-----------------------

The master site for this and related repos is the [Macroassembler-AS
organization on GitHub][ghmas]. Issues and PRs for this repo should be
filed in the [`Macroassembler-AS/asl-releases`][ghmasrel] project
there.

The original import code was written by [Kuba Ober][KubaO],
<kuba@mareimbrium.org>. [Curt J. Sampson][0cjs] contributed this
documentation and is currently doing regular imports of new ASL
versions.



<!-------------------------------------------------------------------->
[asl]: http://john.ccac.rwth-aachen.de:8000/as/
[src]: http://john.ccac.rwth-aachen.de:8000/ftp/as/source/c_version/

[ghmas]: https://github.com/Macroassembler-AS
[ghmasrel]: https://github.com/Macroassembler-AS/asl-releases
[KubaO]: https://github.com/KubaO
[0cjs]: https://github.com/0cjs
[8bitdev]: https://github.com/0cjs/8bitdev
