OpenSim Core modified to enable the use of Algorithmic Differentiation 
======================================================================
 [![Travis][buildstatus_image_travis]][travisci] [![Appveyor][buildstatus_image_appveyor]][appveyorci] [![ZenHub][zenhub_image]][zenhub]
 
**NOTE: This repository has been forked from [Opensim's master source code](https://github.com/opensim-org/opensim-core). Changes have been
made to enable the use of algorithmic differentiation. Consequently, some features of OpenSim have been disabled. Please rely on the original
OpenSim's source code if you do not intend to exploit algorithmic differentiation when solving trajectory optimization problems with our framework.
In addition, please make sure you verify your results. We cannot guarantee that our changes did not affect the original code.**

OpenSim is a software that lets users develop models of musculoskeletal structures and create dynamic simulations of movement. In this work,
we have expanded OpenSim and created a framework for solving trajectory optimization problems using direct collocation methods.
This framework relies on OpenSim for the musculoskeletal structures and multibody dynamics models and on [CasADi](https://web.casadi.org/) for the
nonlinear optimization and algorithmic differentiation. To enable the use of algorithmic differentiation in OpenSim, we have developed a tool named
Recorder that we integrated as part of a modified version of Simbody. More information about this framework and Recorder can be found in this publication.

Solving trajectory optimization problems with our framework allows generating computationally efficient predictive simulations of movement.
For example, you can produce the following predictive simulation of walking with a complex musculoskeletal models (29 degrees of freedom, 92 muscles,
and 6 contact spheres per foot) in only about 20 minutes of CPU time on a single core of a standard laptop computer:

![Predictive simulation of human walking by Antoine Falisse (doi:10.1098/rsif.2019.0402)](doc/images/opensim_predwalking.gif)

More information about how to generate such predictive simulations can be found in [this publication](https://royalsocietypublishing.org/doi/10.1098/rsif.2019.0402).

Brief overview of the framework
-------------------------------

Solving trajectory optimization problems with our framework consists of different steps:

* Build the source code of the modified versions of OpenSim and Simbody that enable the use of algorithmic differentiation ([On Windows using Microsoft Visual Studio](#on-windows-using-visual-studio)).

* Build the OpenSim code intended to be used when formulating the trajectory optimization problem ([Build external functions](#build-external-functions)). For instance, this code may perform inverse dynamics with joint states and controls as input and joint torques as output. We provide a series of examples of how this code may look like in the folder External_Functions. Among them is the code used for generating the predictive simulation in the animation above. We will refer to such code as an external function. You should build this code as an executable.

* Run the executable ([Run executable](#run-executable)). This will generate a MATLAB file, named by default 'foo.m'. This file contains the expression graph of the external function in a format that CasADi can interpret. Expression graphs are at the core of algorithmic differentiation.

* Generate C-code with CasADi ([Generate C-code](#generate-c-code)). From the expression graph generated in the previous step, CasADi can generate C-code allowing to evaluate the (external) function and its derivatives. To generate the C-code, we rely on the code generation feature of CasADi through a few MATLAB commands. We provide a series of examples of how this should be done in the folder cgeneration (details below).

* Compile the generated c-code as a Dynamic Link Library (dll) (details below). This dll can then be imported within the CasADi environment when formulating the trajectory optimization problems. 

* Formulate and solve your trajectory optimization problem. [In this repository](https://github.com/antoinefalisse/3dpredictsim), you can find the code used to generate the predictive simulation in the animation above. [At this line](https://github.com/antoinefalisse/3dpredictsim/blob/master/OCP/PredSim_all.m#L435), we import the dll (compiled in the previous step) as an external function in our environment. We then [evaluate this function](https://github.com/antoinefalisse/3dpredictsim/blob/master/OCP/PredSim_all.m#L1161) when formulating our nonlinear programming problem (NLP). When solving the problem, CasADi provides the NLP solver (e.g., IPOPT) with evaluations of the NLP objective function, constraints, objective function gradient, constraint Jacobian, and Hessian of the Lagrangian. CasADi efficiently queries evaluation of the external function and its derivatives to construct the full derivative matrices.

Building from the source code
-----------------------------

**NOTE -- In all platforms (Windows, OSX, Linux), it is advised to build all OpenSim Dependencies (Simbody, BTK etc) with same *CMAKE_BUILD_TYPE* (Linux) / *CONFIGURATION* (MSVC/Xcode) as OpenSim. For example, if OpenSim is to be built with *CMAKE_BUILD_TYPE/CONFIGURATION* as *Debug*, Simbody, BTK and all other OpenSim dependencies also should be built with *CMAKE_BUILD_TYPE/CONFIGURATION* as *Debug*. Failing to do so *may* result in mysterious runtime errors like 'segfault' in standard c++ library implementation.**

We have developed this project on Windows. We cannot guarantee that this works fine on other platforms although it should in theory. We have kept the original instructions for Mac OSX and Ubuntu:

1. [On Windows using Microsoft Visual Studio](#on-windows-using-visual-studio).
2. [On Mac OSX using Xcode](#on-mac-osx-using-xcode). Need extended instructions? Use [these instructions](#extended-instructions-for-osx).
3. [On Ubuntu using Unix Makefiles](#on-ubuntu-using-unix-makefiles). In a rush? Use [these instructions](#for-the-impatient-ubuntu).


On Windows using Visual Studio
------------------------------

#### Get the dependencies

* **operating system**: Windows 10.
* **cross-platform build system**:
  [CMake](http://www.cmake.org/cmake/resources/software.html) >= 3.2
* **compiler / IDE**: [Visual Studio 2015](https://www.visualstudio.com/). We started this project before the release of Visual Studio 2017 and 2019, you might experience bugs with these later versions so please stick to Visual Studio 2015 (or contribute to the code to make it work with the newer versions :)). You should be able to find Visual Studio Community 2015 after a little bit of googling.
    * *Visual Studio Community 2015* is sufficient and is free for everyone.
    * Visual Studio 2015 does not install C++
      support by default. During the installation you must select
      *Custom*, and check *Programming Languages > Visual C++ > Common Tools
      for Visual C++ 2015*.
      You can uncheck all other boxes. If Visual Studio is installed without C++
      support, CMake will report the following errors:
      
      ```
      The C compiler identification is unknown
      The CXX compiler identification is unknown
      ```
      
      If you have already installed Visual Studio without C++ support, simply
      re-run the installer and select *Modify*. Alternatively, go to
      *File > New > Project...* in Visual Studio, select *Visual C++*, and click
      *Install Visual C++ 2015 Tools for Windows Desktop*.
* **physics engine**: Simbody >= 3.6. Two options:
    * Let OpenSim get this for you using superbuild (see below); much easier!
    * [Build on your own: be careful you need to build the modified version that enables the use of AD](
      https://github.com/antoinefalisse/simbody/tree/AD-recorder#windows-using-visual-studio).
* **C3D file support**: Biomechanical-ToolKit Core. Two options:
    * Let OpenSim get this for you using superbuild (see below); much easier!
    * [Build on your own](https://github.com/klshrinidhi/BTKCore).
* **command-line argument parsing**: docopt.cpp. Two options:
    * Let OpenSim get this for you using superbuild (see below); much easier!
    * [Build on your own](https://github.com/docopt/docopt.cpp) (no instructions).

#### Download the OpenSim-Core source code modified to enable algorithmic differentiation (OpenSim-AD-Core)

* Clone the opensim-ad-core git repository. We'll assume you clone it into `C:/opensim-ad-core-source`.
  **Be careful that the repository is not on the `master` branch but on the `AD-recorder` branch.** 

  If using a Git Bash or Git Shell, run the following:
  
        $ git clone -b AD-recorder https://github.com/antoinefalisse/opensim-core.git C:/opensim-ad-core-source  
  
  If using TortoiseGit, open Windows Explorer,
  right-click in the window, select **Git Clone...**, and provide the
  following:
    * **URL**: `https://github.com/antoinefalisse/opensim-core.git`.

    * **Directory**: `C:/opensim-ad-core-source`.
    
    * **Checkout the `AD-recorder` branch.**

#### [Optional] Superbuild: download and build OpenSim dependencies
1. Open the CMake GUI.
2. In the field **Where is the source code**, specify
   `C:/opensim-ad-core-source/dependencies`.
3. In the field **Where to build the binaries**, specify a directory under
   which to build dependencies. Let's say this is
   `C:/opensim-ad-core-dependencies-build`.
4. Click the **Configure** button.
    1. Choose the *Visual Studio 14 2015* generator. Make sure
       your build as 64-bit (x64; it's 32-bit by default in later CMake version).
    2. Click **Finish**.
5. Where do you want to install OpenSim dependencies on your computer? Set this
   by changing the `CMAKE_INSTALL_PREFIX` variable. Let's say this is
   `C:/opensim-ad-core-dependencies-install`.
6. Variables named `SUPERBUILD_<dependency-name>` allow you to selectively
   download dependencies. By default, all dependencies are downloaded,
   configured and built.
7. Click the **Configure** button again. Then, click **Generate** to make
   Visual Studio project files in the build directory.
8. Go to the build directory you specified in step 3 using the command:

        cd C:/opensim-ad-core-dependencies-build

9. Use CMake to download, compile and install the dependencies:

        cmake --build . --config RelWithDebInfo

   Alternative values for `--config` in this command are:
   
   * **Debug**: debugger symbols; no optimizations (more than 10x slower).
     Library names end with `_d`.
   * **Release**: no debugger symbols; optimized.
   * **RelWithDebInfo**: debugger symbols; optimized. Bigger but not slower
     than Release; choose this if unsure.
   * **MinSizeRel**: minimum size; optimized.

   You must run this command for each of the configurations you plan to use
   with OpenSim (see below). You should run this command for the release
   configuration *last* to ensure that you use the release version of the
   command-line applications instead of the slow debug versions.
10. If you like, you can now remove the directory used for building
    dependencies (`c:/opensim-core-dependencies-build`).

#### Configure and generate project files

1. Open the CMake GUI.
2. In the field **Where is the source code**, specify `C:/opensim-ad-core-source`.
3. In the field **Where to build the binaries**, specify something like
   `C:/opensim-ad-core-build`, or some other path that is not inside your source
   directory. This is *not* where we are installing OpenSim-Core; see below.
4. Click the **Configure** button.
    1. Choose the *Visual Studio 14 2015* generator. Make sure
       your build as 64-bit (x64; it's 32-bit by default in later CMake version).
    2. Click **Finish**.
5. Where do you want to install OpenSim-AD-Core on your computer? Set this by
   changing the `CMAKE_INSTALL_PREFIX` variable. We'll assume you set it to
   `C:/opensim-ad-core-install`. If you choose a different installation location, make
   sure to use *yours* where we use `C:/opensim-ad-core-install` below.
6. Tell CMake where to find dependencies. This depends on how you got them.
    * Superbuild: Set the variable `OPENSIM_DEPENDENCIES_DIR` to the root
      directory you specified with superbuild for installation of dependencies.
      In our example, it would be `c:/opensim-ad-core-dependencies-install`.
    * Obtained on your own:
        1. Simbody: Set the `SIMBODY_HOME` variable to where you installed
           Simbody (e.g., `C:/Simbody-ad-install`).
        2. BTK: Set the variable `BTK_DIR` to the directory containing
           `BTKConfig.cmake`. If the root directory of your BTK installation is
           `C:/BTKCore-install`, then set this variable to
           `C:/BTKCore-install/share/btk-0.4dev`.
        3. docopt.cpp. Set the variable `docopt_DIR` to the directory
           containing `docopt-config.cmake`. If the root directory of your 
           docopt.cpp installation is `C:/docopt.cpp-install`, then set this 
           variable to `C:/docopt.cpp-install/lib/cmake`.
7. Set the remaining configuration options.
    * `WITH_RECORDER` to compile OpenSim modified to enable the use of algorithmic differentiation.
    * `BUILD_API_EXAMPLES` to compile C++ API examples. Note that most examples will not work with this new version of OpenSim. You could turn this off.
    * `BUILD_TESTING` to ensure that OpenSim works correctly. Note that most tests will fail with this new version of OpenSim. **Nevertheless, you
    should turn this on to build the external functions.**
    * `BUILD_JAVA_WRAPPING` if you want to access OpenSim through MATLAB or
      Java; see dependencies above. Please turn this off (not relevant for our applications).
    * `BUILD_PYTHON_WRAPPING` if you want to access OpenSim through Python; see
      dependencies above. CMake sets `PYTHON_*` variables to tell you the
      Python version used when building the wrappers. Please turn this off (not relevant for our applications).
    * `OPENSIM_PYTHON_VERSION` to choose if the Python wrapping is built for
      Python 2 or Python 3. Please turn this off (not relevant for our applications).
    * `BUILD_API_ONLY` if you don't want to build the command-line applications.
8. Click the **Configure** button again. Then, click **Generate** to make
   Visual Studio project files in the build directory.

#### Build

1. Open `C:/opensim-ad-core-build/OpenSim.sln` in Visual Studio.
2. Select your desired *Solution configuration* from the drop-down at the top.
    * **Debug**: debugger symbols; no optimizations (more than 10x slower).
      Library names end with `_d`.
    * **Release**: no debugger symbols; optimized.
    * **RelWithDebInfo**: debugger symbols; optimized. Bigger but not slower
      than Release; choose this if unsure.
    * **MinSizeRel**: minimum size; optimized.

    You at least want release libraries (the last 3 count as release), but you
    can have debug libraries coexist with them. To do this, go through the
    installation process twice, once for each of the two configurations. You
    should install the release configuration *last* to ensure that you use the
    release version of the command-line applications instead of the slow debug
    versions.
4. Build the libraries. **For our applications, we only need to build osimCommon and osimSimulation, building all libraries will fail.** 
   Right-click on osimCommon in the folder Libraries and select **Build**. Process in the same way for osimSimulation.
5. Copy Simbody DLLs. Right-click on Copy Simbody DLLs and select **Build**.
   
Build external functions
------------------------

In the folder **External_Functions**, you can find a series of example external functions we used for different applications. To add your own external
function, take a look at an example in `C:/opensim-ad-core/OpenSim/External_Functions`. Don't forget to edit the CMakeLists. Your new external function
will appear in Visual Studio after re-configuring through CMake. For the rest of the instructions, we will use the example **PredSim**.

1. Build the external function. Right-click on PredSim and select **Build**. To skip the next step (Run executable), you can also right-click on PredSim, select
**Set as StartUp Project**, click on Debug (toolbar) and click on **Start Without Debugging**. If you followed the second approach, you should find
a MATLAB file foo.m in the folder `C:/opensim-ad-core-build/OpenSim/External_Functions/PredSim`.


Run executable
--------------

If you haven't run the executable yet (e.g., through **Start Without Debugging**):
1. Open `C:/opensim-ad-core-build/RelWithDebInfo` in a terminal window (assuming you are in RelWithDebInfo mode):

        cd C:/opensim-ad-core-build/RelWithDebInfo
    
2. Run the executable:

        PredSim.exe
    
You should find a MATLAB file foo.m in the folder `C:/opensim-ad-core-build/RelWithDebInfo`.

Generate C-code
---------------

   

On Mac OSX using Xcode (instructions have not yet been updated for this modified version of OpenSim, please adjust based on the Windows instructions)
-----------------------------------------------------------------------------------------------------------------------------------------------------

### For Mac OSX 10.11 El Capitan
Get **Xcode** from the App store. Open **Xcode** and *Agree* to license agreement. To *Agree* to to the license agreement, you may need to type in **Terminal**:
```shell 
sudo xcodebuild -license
``` 
If you already have **Xcode**, update it to 7.3, or the latest version.

Then, in **Terminal**, copy and paste commands below, line by line, one at a time. Be sure the output doesn't contain errors.
```shell
/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
brew install cmake swig
brew cask install java
git clone https://github.com/opensim-org/opensim-core.git
mkdir opensim_dependencies_build
cd opensim_dependencies_build
cmake ../opensim-core/dependencies \
      -DCMAKE_INSTALL_PREFIX="~/opensim_dependencies_install" \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo
make -j8
cd ..
mkdir opensim_build
cd opensim_build
cmake ../opensim-core \
      -DCMAKE_INSTALL_PREFIX="~/opensim_install" \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DBUILD_PYTHON_WRAPPING=ON \
      -DBUILD_JAVA_WRAPPING=ON \
      -DOPENSIM_DEPENDENCIES_DIR="~/opensim_dependencies_install" \
      -DWITH_BTK=ON
make -j8
ctest -j8
```


### Extended Instructions for OSX

#### Get the dependencies

* **operating system**: Mac OSX 10.11 El Capitan.
* **cross-platform build system**:
  [CMake](http://www.cmake.org/cmake/resources/software.html) >= 3.2
* **compiler / IDE**: [Xcode](https://developer.apple.com/xcode/) >= 7.3 (the latest version), through
  the Mac App Store.
* **physics engine**: Simbody >= 3.6. Two options:
  * Let OpenSim get this for you using superbuild (see below).
  * [Build on your own](https://github.com/simbody/simbody#installing).
* **C3D file support**: Biomechanical-ToolKit Core. Two options:
  * Let OpenSim get this for you using superbuild (see below).
  * [Build on your own](https://github.com/klshrinidhi/BTKCore).
* **command-line argument parsing**: docopt.cpp. Two options:
    * Let OpenSim get this for you using superbuild (see below); much easier!
    * [Build on your own](https://github.com/docopt/docopt.cpp) (no instructions).
* **API documentation** (optional):
  [Doxygen](http://www.stack.nl/~dimitri/doxygen/download.html) >= 1.8.6
* **version control** (optional): git.
    * Xcode Command Line Tools gives you git on the command line.
    * [GitHub for Mac](https://mac.github.com), for a simple-to-use GUI.
* **Bindings** (optional): [SWIG](http://www.swig.org/) 3.0.8
    * **MATLAB scripting** (optional): [Java development kit][java] >= 1.7.
        * Note: Older versions of MATLAB may use an older version of JVM. Run
                'ver' in MATLAB to check MATLAB's JVM version (must be >= 1.7).
    * **Python scripting** (optional): Python 2 >= 2.7 or Python 3 >= 3.5
        * Mac OSX comes with Python, but you could also use:
        * [`brew install python`](http://brew.sh),
        * [Anaconda](https://store.continuum.io/cshop/anaconda/)

You can get most of these dependencies using [Homebrew](http://brew.sh):

    $ brew install cmake doxygen swig

#### Download the OpenSim-Core source code

* Method 1; If you want to get going quickly, download the source code from
  https://github.com/opensim-org/opensim-core/releases, for the version of
  OpenSim you want. We'll assume you unzipped the source code into
  `~/opensim-core-source`.
* Method 2: If you plan on updating your OpenSim installation or you want to
  contribute back to the project, clone the opensim-core git repository into
  `~/opensim-core-source`. Run the following in a terminal, or
  find a way to run the equivalent commands in a GUI client:

        $ git clone https://github.com/opensim-org/opensim-core.git ~/opensim-core-source

  This will give you a bleeding-edge version of OpenSim-Core.

#### [Optional] Superbuild: download and build OpenSim dependencies
1. Open the CMake GUI.
2. In the field **Where is the source code**, specify
   `~/opensim-core-source/dependencies`.
3. In the field **Where to build the binaries**, specify a directory under
   which to build dependencies. Let's say this is
   `~/opensim-core-dependencies-build`.
4. Click the **Configure** button. Choose **Xcode**. Click **Finish**.
5. Where do you want to install OpenSim dependencies on your computer? Set this
   by changing the `CMAKE_INSTALL_PREFIX` variable. Let's say this is
   `~/opensim-core-dependencies-install`.
6. Variables named `SUPERBUILD_<dependency-name>` allow you to selectively
   download dependencies. By default, all dependencies are downloaded,
   configured and built.
7. Click the **Configure** button again. Then, click **Generate** to make Xcode
   files in the build directory.
8. Open `~/opensim-core-dependencies/build/OpenSimDependencies.xcodeproj` in
   Xcode.
9. Choose your **Build Configuration** for the **ALL_BUILD** Scheme by pressing
   `Command-Shift ,` (or, `Command-LessThan`), or navigating to **Product ->
   Scheme -> Edit Scheme...**; and changing the **Build Configuration** field.
    * **Debug**: debugger symbols; no optimizations (more than 10x slower).
    Library names end with `_d`.
    * **Release**: no debugger symbols; optimized.
    * **RelWithDebInfo**: debugger symbols; optimized. Bigger but not slower
    than Release; choose this if unsure.
    * **MinSizeRel**: minimum size; optimized.

    You must build each of the configurations you plan to use with OpenSim (see
    below). You should install the release configuration *last* to ensure that
    you use the release version of the command-line applications instead of the
    slow debug versions.

10. Compile. Run the Scheme **ALL_BUILD** by clicking the play button in the
   upper left. If necessary, change the build configuration (previous step) and
   run **ALL_BUILD** again.

#### Configure and generate project files

1. Open the CMake GUI.
2. In the field **Where is the source code**, specify `~/opensim-core-source`.
3. In the field **Where to build the binaries**, specify something like
   `~/opensim-core-build`, or some other path that is not inside your source
   directory. This is *not* where we are installing OpenSim-Core; see below.
4. Click the **Configure** button. Choose **Xcode**. Click **Finish**.
5. Where do you want to install OpenSim-Core on your computer? Set this by
   changing the `CMAKE_INSTALL_PREFIX` variable. We'll assume you set it to
   `~/opensim-core`. If you choose a different installation location, make
   sure to use *yours* where we use `~/opensim-core` below. You should *not*
   use `/usr/`, `/usr/local/`, etc. (because our installation does not yet
   conform to the [FHS](http://www.pathname.com/fhs/)).
6. Tell CMake where to find dependencies. This depends on how you got them.
    * Superbuild: Set the variable `OPENSIM_DEPENDENCIES_DIR` to the root
      directory you specified with superbuild for installation of dependencies.
      In our example, it would be `~/opensim-core-dependencies-install`.
    * Obtained on your own:
        1. Simbody: Set the `SIMBODY_HOME` variable to where you installed
           Simbody (e.g., `~/simbody`). If you installed Simbody using `brew`,
           then CMake will find Simbody automatically.
        2. BTK: Set the `BTK_DIR` variable to the directory containing
           `BTKConfig.cmake`. If you installed BTK in `~/BTKCore-install`, then
           set `BTK_DIR` to `~/BTKCore-install/share/btk-0.4dev`
        3. docopt.cpp. Set the variable `docopt_DIR` to the directory
           containing `docopt-config.cmake`. If the root directory of your 
           docopt.cpp installation is `~/docopt.cpp-install`, then set this 
           variable to `~/docopt.cpp-install/lib/cmake`.
7. Set the remaining configuration options.
    * `BUILD_API_EXAMPLES` to compile C++ API examples.
    * `BUILD_TESTING` to ensure that OpenSim works correctly. The tests take a
      while to build; if you want to build OpenSim quickly, you can turn this
      off.
    * `BUILD_JAVA_WRAPPING` if you want to access OpenSim through MATLAB or
      Java; see dependencies above.
    * `BUILD_PYTHON_WRAPPING` if you want to access OpenSim through Python; see
      dependencies above. CMake sets `PYTHON_*` variables to tell you the
      Python version used when building the wrappers.
    * `OPENSIM_PYTHON_VERSION` to choose if the Python wrapping is built for
      Python 2 or Python 3.
    * `BUILD_API_ONLY` if you don't want to build the command-line applications.
8. Click the **Configure** button again. Then, click **Generate** to create
   Xcode project files in the build directory.

#### Build and install

1. Open `~/opensim-core-build/OpenSim.xcodeproj` in Xcode.
2. Choose your **Build Configuration** for the **ALL_BUILD** Scheme by pressing
   `Command-Shift ,` (or, `Command-LessThan`), or navigating to **Product ->
   Scheme -> Edit Scheme...**; and changing the **Build Configuration** field.
    * **Debug**: debugger symbols; no optimizations (more than 10x slower).
    Library names end with `_d`.
    * **Release**: no debugger symbols; optimized.
    * **RelWithDebInfo**: debugger symbols; optimized. Bigger but not slower
    than Release; choose this if unsure.
    * **MinSizeRel**: minimum size; optimized.

    You at least want release libraries (the last 3 count as release), but you
    can have debug libraries coexist with them. To do this, go through the
    installation process twice, once for each of the two configurations. You
    should install the release configuration *last* to ensure that you use the
    release version of the command-line applications instead of the slow debug
    versions.

3. Compile. Run the Scheme **ALL_BUILD** by clicking the play button in the
   upper left.
4. Test. Click on **ALL_BUILD** in the upper left, and select
   **RUN_TESTS_PARALLEL**. Change the **Build Configuration** of this Scheme to
   the same as you used for **ALL_BUILD** (using the same instructions as
   above). Click the play button.
5. Build the API documentation. This is optional, and you can only do this if
   you have Doxygen. Click on the current Scheme (**RUN_TESTS_PARALLEL**) and
   select **doxygen**. Click the play button.
6. Install. Click on the current Scheme (**RUN_TESTS_PARALLEL** or
   **doxygen**), and select **install**. Click the play button.

#### Set environment variables

1. **Executables**. If you want to run OpenSim-Core's executables from
   anywhere on your computer, you must update your PATH. Open a terminal and
   type:

        $ echo 'export PATH=~/opensim-core/bin:$PATH' >> ~/.bash_profile

Your changes will only take effect in new terminal windows.


On Ubuntu using Unix Makefiles (instructions have not yet been updated for this modified version of OpenSim, please adjust based on the Windows instructions)
-------------------------------------------------------------------------------------------------------------------------------------------------------------

#### Get the dependencies

Most dependencies can be obtained via the Ubuntu software repositories;
especially if you are using Ubuntu 16.04 or later. On each line below, we show
the Ubuntu package names for the dependencies. You can find instructions for
specific Ubuntu versions under 'For the impatient' below.

* **cross-platform build system**:
  [CMake](http://www.cmake.org/cmake/resources/software.html) >= 3.2;
  `cmake-gui`. 
* **compiler**: [gcc](http://gcc.gnu.org) >= 4.9; `g++-4.9`, or
  [Clang](http://clang.llvm.org) >= 3.4; `clang-3.4`.
* **physics engine**: Simbody >= 3.6. Two options:
  * Let OpenSim get this for you using superbuild (see below).
  * [Build on your own](https://github.com/simbody/simbody#installing).
* **C3D file support**: Biomechanical-ToolKit Core. Two options:
  * Let OpenSim get this for you using superbuild (see below).
  * [Build on your own](https://github.com/klshrinidhi/BTKCore).
* **command-line argument parsing**: docopt.cpp. Two options:
    * Let OpenSim get this for you using superbuild (see below); much easier!
    * [Build on your own](https://github.com/docopt/docopt.cpp) (no instructions).
* **API documentation** (optional):
  [Doxygen](http://www.stack.nl/~dimitri/doxygen/download.html) >= 1.8.6;
  `doxygen`.
* **version control** (optional): git; `git`.
* **Bindings** (optional): [SWIG](http://www.swig.org/) 3.0.8; `swig`.
    * **MATLAB scripting** (optional): [Java development kit][java] >= 1.7;
      `openjdk-6-jdk` or `openjdk-7-jdk`.
        * Note: Older versions of MATLAB may use an older version of JVM. Run
                'ver' in MATLAB to check MATLAB's JVM version (must be >= 1.7).
    * **Python scripting** (optional): Python 2 >= 2.7 or Python 3 >= 3.5; `python-dev`.

For example, you could get the required dependencies (except Simbody) via:

    $ sudo apt-get install cmake-gui g++-4.9

And you could get all the optional dependencies via:

    $ sudo apt-get install doxygen git swig openjdk-7-jdk python-dev

#### Download the OpenSim-Core source code

* Method 1; If you want to get going quickly, download the source code from
  https://github.com/opensim-org/opensim-core/releases, for the version of
  OpenSim you want. We'll assume you unzipped the source code into
  `~/opensim-core-source`.
* Method 2: If you plan on updating your OpenSim installation or you want to
  contribute back to the project, clone the opensim-core git repository into
  `C:/opensim-core-source`. Run the following in a terminal:

        $ git clone https://github.com/opensim-org/opensim-core.git ~/opensim-core-source

  This will give you a bleeding-edge version of OpenSim-Core.

#### [Optional] Superbuild: download and build OpenSim dependencies
1. Open the CMake GUI.
2. In the field **Where is the source code**, specify
   `~/opensim-core-source/dependencies`.
3. In the field **Where to build the binaries**, specify a directory under
   which to build dependencies. Let's say this is
   `~/opensim-core-dependencies-build`.
4. Click the **Configure** button. Choose *Unix Makefiles*. Click **Finish**.
5. Where do you want to install OpenSim dependencies on your computer? Set this
   by changing the `CMAKE_INSTALL_PREFIX` variable. Let's say this is
   `~/opensim-core-dependencies-install`.
6. Variables named `SUPERBUILD_<dependency-name>` allow you to selectively
   download dependencies. By default, all dependencies are downloaded,
   configured and built.
7. Choose your build type by setting `CMAKE_BUILD_TYPE` to one of the following:
    * **Debug**: debugger symbols; no optimizations (more than 10x slower).
    Library names end with `_d`.
    * **Release**: no debugger symbols; optimized.
    * **RelWithDebInfo**: debugger symbols; optimized. Bigger but not slower
    than Release; choose this if unsure.
    * **MinSizeRel**: minimum size; optimized.

    You must perform the superbuild procedure for each of the
    build types you plan to use with OpenSim (see below). You might want to
    use different build directories for each build type, though you can use
    the same install directory for all build types. You should install the
    release build type *last* to ensure that you use the release version of
    the command-line applications instead of the slow debug versions.
8. Click the **Configure** button again. Then, click **Generate** to make Unix
   Makefiles in the build directory.
9. Open a terminal and navigate to the build directory.

        $ cd ~/opensim-core-dependencies-build

3. Compile. Use the `-jn` flag to build using `n` concurrent jobs (potentially
   in parallel); this will greatly speed up your build. For example:

        $ make -j8
11. If necessary, repeat this whole procedure for other build types.

#### Configure and generate project files

1. Open the CMake GUI.
2. In the field **Where is the source code**, specify `~/opensim-core-source`.
3. In the field **Where to build the binaries**, specify something like
   `~/opensim-core-build`, or some other path that is not inside your source
   directory. This is *not* where we are installing OpenSim-Core; see below.
4. Click the **Configure** button. Choose *Unix Makefiles*. Click **Finish**.
5. Where do you want to install OpenSim-Core on your computer? Set this by
   changing the `CMAKE_INSTALL_PREFIX` variable. We'll assume you set it to
   `~/opensim-core`. If you choose a different installation location, make
   sure to use *yours* where we use `~/opensim-core` below. You should *not*
   use `/usr/`, `/usr/local/`, etc. (because our installation does not yet
   conform to the [FHS](http://www.pathname.com/fhs/)), but
   [`/opt/`](http://www.tldp.org/LDP/Linux-Filesystem-Hierarchy/html/opt.html)
   is okay.
6. Tell CMake where to find dependencies. This depends on how you got them.
    * Superbuild: Set the variable `OPENSIM_DEPENDENCIES_DIR` to the root
      directory you specified with superbuild for installation of dependencies.
      In our example, it would be `~/opensim-core-dependencies-install`.
    * Obatained on your own:
        1. Simbody: Set the `SIMBODY_HOME` variable to where you installed
           Simbody (e.g., `~/simbody`).
        2. BTK: Set the `BTK_DIR` variable to the directory containing
           `BTKConfig.cmake`. If you installed BTK in `~/BTK-install`, then set
           `BTK-DIR` to `~/BTK-install/share/btk-0.4dev`.
        3. docopt.cpp. Set the variable `docopt_DIR` to the directory
           containing `docopt-config.cmake`. If the root directory of your 
           docopt.cpp installation is `~/docopt.cpp-install`, then set this 
           variable to `~/docopt.cpp-install/lib/cmake`.
7. Choose your build type by setting `CMAKE_BUILD_TYPE` to one of the following:
    * **Debug**: debugger symbols; no optimizations (more than 10x slower).
    Library names end with `_d`.
    * **Release**: no debugger symbols; optimized.
    * **RelWithDebInfo**: debugger symbols; optimized. Bigger but not slower
    than Release; choose this if unsure.
    * **MinSizeRel**: minimum size; optimized.

    You at least want release libraries (the last 3 count as release), but you
    can have debug libraries coexist with them. To do this, go through the
    installation process twice, once for each of the two build types. It is
    typical to use a different build directory for each build type (e.g.,
    `~/opensim-core-build-debug` and `~/opensim-core-build-release`). You
    should install the release build type *last* to ensure that you use the
    release version of the command-line applications instead of the slow debug
    versions.
8. Set the remaining configuration options.
    * `BUILD_API_EXAMPLES` to compile C++ API examples.
    * `BUILD_TESTING` to ensure that OpenSim works correctly. The tests take a
      while to build; if you want to build OpenSim quickly, you can turn this
      off.
    * `BUILD_JAVA_WRAPPING` if you want to access OpenSim through MATLAB or
      Java; see dependencies above.
    * `BUILD_PYTHON_WRAPPING` if you want to access OpenSim through Python; see
      dependencies above.
    * `OPENSIM_PYTHON_VERSION` to choose if the Python wrapping is built for
      Python 2 or Python 3.
    * `BUILD_API_ONLY` if you don't want to build the command-line applications.
    * `OPENSIM_COPY_DEPENDENCIES` to decide if Simbody and BTK are copied into
      the OpenSim installation; you want this off if you're installing OpenSim
      into `/usr/` or `/usr/local/`.
9. Click the **Configure** button again. Then, click **Generate** to create
   Makefiles in the build directory.

#### Build and install

1. Open a terminal and navigate to the build directory.

        $ cd ~/opensim-core-build

2. Build the API documentation. This is optional, and you can only do this if
   you have Doxygen.

        $ make doxygen

3. Compile. Use the `-jn` flag to build using `n` concurrent jobs (potentially
   in parallel); this will greatly speed up your build. For example:

        $ make -j8

4. Run the tests.

        $ ctest -j8

5. Install (to `~/opensim-core`).

        $ make -j8 install

#### Set environment variables

1. **Executables**. Add OpenSim-Core's executables to the path so you can
   access them from any directory on your computer.

        $ echo 'export PATH=~/opensim-core/bin:$PATH' >> ~/.bashrc

Your changes will only take effect in new terminal windows.

#### For the impatient (Ubuntu)
##### Ubuntu 14.04 Trusty Tahr
In **Terminal** --
```shell
sudo add-apt-repository --yes ppa:george-edison55/cmake-3.x
sudo apt-add-repository --yes ppa:fenics-packages/fenics-exp
sudo apt-get update
sudo apt-get --yes install git cmake cmake-curses-gui clang-3.6 \
                           freeglut3-dev libxi-dev libxmu-dev \
                           liblapack-dev swig3.0 python-dev \
                           openjdk-7-jdk
sudo rm -f /usr/bin/cc /usr/bin/c++
sudo ln -s /usr/bin/clang-3.6 /usr/bin/cc
sudo ln -s /usr/bin/clang++-3.6 /usr/bin/c++
export JAVA_HOME=/usr/lib/jvm/java-7-openjdk-amd64
git clone https://github.com/opensim-org/opensim-core.git
mkdir opensim_dependencies_build
cd opensim_dependencies_build
cmake ../opensim-core/dependencies/ \
      -DCMAKE_INSTALL_PREFIX='~/opensim_dependencies_install' \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo
make -j8
cd ..
mkdir opensim_build
cd opensim_build
cmake ../opensim-core \
      -DCMAKE_INSTALL_PREFIX="~/opensim_install" \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DOPENSIM_DEPENDENCIES_DIR="~/opensim_dependencies_install" \
      -DBUILD_PYTHON_WRAPPING=ON \
      -DBUILD_JAVA_WRAPPING=ON \
      -DWITH_BTK=ON
make -j8
ctest -j8
make -j8 install
 ```
##### Ubuntu 15.10 Wily Werewolf
In **Terminal** --
```shell
sudo apt-add-repository --yes ppa:fenics-packages/fenics
sudo apt-get update
sudo apt-get --yes install git cmake cmake-curses-gui \
                           freeglut3-dev libxi-dev libxmu-dev \
                           liblapack-dev swig3.0 python-dev \
                           openjdk-8-jdk
export JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64
git clone https://github.com/opensim-org/opensim-core.git
mkdir opensim_dependencies_build
cd opensim_dependencies_build
cmake ../opensim-core/dependencies/ \
      -DCMAKE_INSTALL_PREFIX='~/opensim_dependencies_install' \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo
make -j8
cd ..
mkdir opensim_build
cd opensim_build
cmake ../opensim-core \
      -DCMAKE_INSTALL_PREFIX="~/opensim_install" \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DOPENSIM_DEPENDENCIES_DIR="~/opensim_dependencies_install" \
      -DBUILD_PYTHON_WRAPPING=ON \
      -DBUILD_JAVA_WRAPPING=ON \
      -DWITH_BTK=ON
make -j8
ctest -j8
make -j8 install
```
##### Ubuntu 16.04 Xenial Xerus AND Ubuntu 16.10 Yakkety Yak
In **Terminal** --
```shell
sudo apt-get update
sudo apt-get --yes install git cmake cmake-curses-gui \
                           freeglut3-dev libxi-dev libxmu-dev \
                           liblapack-dev swig python-dev \
                           openjdk-8-jdk
export JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64
git clone https://github.com/opensim-org/opensim-core.git
mkdir opensim_dependencies_build
cd opensim_dependencies_build
cmake ../opensim-core/dependencies/ \
      -DCMAKE_INSTALL_PREFIX='~/opensim_dependencies_install' \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo
make -j8
cd ..
mkdir opensim_build
cd opensim_build
cmake ../opensim-core \
      -DCMAKE_INSTALL_PREFIX="~/opensim_install" \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DOPENSIM_DEPENDENCIES_DIR="~/opensim_dependencies_install" \
      -DBUILD_PYTHON_WRAPPING=ON \
      -DBUILD_JAVA_WRAPPING=ON \
      -DWITH_BTK=ON
make -j8
ctest -j8
make -j8 install
```
Note: You may need to add `<FULL-DIR>/opensim_install/bin` to your PATH variable as per [these instructions](#set-environment-variables-2).  
Example: If opensim_install is in your home directory:

        $ echo 'export PATH=~/opensim_install/bin:$PATH' >> ~/.bashrc
 

[buildstatus_image_travis]: https://travis-ci.org/opensim-org/opensim-core.svg?branch=master
[travisci]: https://travis-ci.org/opensim-org/opensim-core
[buildstatus_image_appveyor]: https://ci.appveyor.com/api/projects/status/i4wxnmx9jlk69kge/branch/master?svg=true
[appveyorci]: https://ci.appveyor.com/project/opensim-org/opensim-core/branch/master
[zenhub_image]: https://raw.githubusercontent.com/ZenHubIO/support/master/zenhub-badge.png
[zenhub]: https://zenhub.com

[running_gif]: doc/images/opensim_running.gif
[simple_example_gif]: doc/images/opensim_double_pendulum_muscle.gif
[java]: http://www.oracle.com/technetwork/java/javase/downloads/index.html
