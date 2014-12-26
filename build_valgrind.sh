#!/usr/bin/env bash

#set -x 

function extract()
{
	if [ -f "$1" ] ; then
	case "$1" in
		*.tar.bz2)   tar xvjf "$1"     ;;
		*.tar.gz)    tar xvzf "$1"     ;;
		*.bz2)       bunzip2 "$1"      ;;
		*.rar)       unrar x "$1"      ;;
		*.gz)        gunzip "$1"       ;;
		*.tar)       tar xvf "$1"      ;;
		*.tbz2)      tar xvjf "$1"     ;;
		*.tgz)       tar xvzf "$1"     ;;
		*.zip)       unzip "$1"        ;;
		*.Z)         uncompress "$1"   ;;
		*.7z)        7z x "$1"         ;;
		*)           echo "$1 cannot be extracted via >extract<" ;;
		esac
	else
		echo "'$1' is not a valid file"
			fi
}

RUN_HELLO_JNI_THROUGH_VALGRIND=false
VALGRIND_VERSION="3.10.0"
VALGRIND_EXTENSION=".tar.bz2"
VALGRIND_DIRECTORY="valgrind-${VALGRIND_VERSION}"
VALGRIND_TARBALL="valgrind-${VALGRIND_VERSION}${VALGRIND_EXTENSION}"

# Only download Valgrind tarball again if not already downloaded
if [[ ! -f "${VALGRIND_TARBALL}" ]]; then
wget -v -nc "http://valgrind.org/downloads/${VALGRIND_TARBALL}"
fi

# Only extract Valgrind tarball again if not already extracted
if [[ ! -d "$VALGRIND_DIRECTORY" ]]; then
extract "$VALGRIND_TARBALL"
fi

# Ensure ANDROID_NDK_HOME is set
if [[ ! -z "$ANDROID_NDK_HOME" ]]; then
export ANDROID_NDK_HOME="$HOME/Software/Android/android-ndk-r10c"
export ANDROID_NDK_HOME="/Users/yihuang/Development/android-ndk-r10c"
fi

# Ensure ANDOID_SDK_HOME is set
if [[ ! -z "$ANDROID_SDK_HOME" ]]; then
export ANDROID_SDK_HOME="$HOME/Software/Android/android-sdk/"
export ANDROID_SDK_HOME="/Users/yihuang/Development/adt-bundle-mac-x86_64-20140702/sdk"
fi

if [[ ! -d "$VALGRIND_DIRECTORY" ]];
then
echo "Problem with extracting Valgrind from $VALGRIND_TARBALL into $VALGRIND_DIRECTORY!!!"
exit -1
fi

# Move to extracted directory
cd "$VALGRIND_DIRECTORY"

# ARM Toolchain
ARCH_ABI="arm-linux-androideabi-4.8"
echo $ANDROID_NDK_HOME
echo $ARCH_ABI
export AR="$ANDROID_NDK_HOME/toolchains/${ARCH_ABI}/prebuilt/darwin-x86_64/bin/arm-linux-androideabi-ar"
export LD="$ANDROID_NDK_HOME/toolchains/${ARCH_ABI}/prebuilt/darwin-x86_64/bin/arm-linux-androideabi-ld"
export CC="$ANDROID_NDK_HOME/toolchains/${ARCH_ABI}/prebuilt/darwin-x86_64/bin/arm-linux-androideabi-gcc"
export CXX="$ANDROID_NDK_HOME/toolchains/${ARCH_ABI}/prebuilt/darwin-x86_64/bin/arm-linux-androideabi-g++"
echo $AR
echo $LD
echo $CC
echo $CXX

[[ ! -d "$ANDROID_NDK_HOME" || ! -f "$AR" || ! -f "$LD" || ! -f "$CC" || ! -f "$CXX" ]] && echo "Make sure AR, LD, CC, CXX variables are defined correctly. Ensure ANDROID_NDK_HOME is defined also" && exit -1

# Configure build
export HWKIND="nexus_s"
ANDROID_PLATFORM=android-18
export CPPFLAGS="--sysroot=$ANDROID_NDK_HOME/platforms/${ANDROID_PLATFORM}/arch-arm -DANDROID_HARDWARE_$HWKIND"
export CFLAGS="--sysroot=$ANDROID_NDK_HOME/platforms/${ANDROID_PLATFORM}/arch-arm"

# BUG: For some reason file command is unable to detect if the file does not exist with ! -f , it says it doesn't exist even when it does!!!
BUILD=false
echo Inst/data/local/Inst/bin/valgrind
mkdir -p Inst/data/local/Inst/bin/valgrind
if [[ "Inst/data/local/Inst/bin/valgrind" = *"No such file or directory"* ]]; then
BUILD=true
fi
echo "BUILD=$BUILD"
BUILD=true
if [[ "$BUILD" = true ]];
then
./configure --prefix="/data/local/Inst" \
		      --host="armv7-unknown-linux" \
		      --target="armv7-unknown-linux" \
		      --with-tmpdir="/sdcard "

		      [[ $? -ne 0 ]] && echo "Can't configure!" && exit -1

# Determine the number of jobs (commands) to be run simultaneously by GNU Make
# NO_CPU_CORES=$(grep -c ^processor /proc/cpuinfo)

#	if [ $NO_CPU_CORES -le 8 ]; then
#JOBS=$(($NO_CPU_CORES+1))
#	else
#	JOBS=${NO_CPU_CORES}
#	fi
JOB=4

# Compile Valgrind 
	make -j "${JOBS}"

	[[ $? -ne 0 ]] && echo "Can't compile!" && exit -1

# Install Valgrind locally
	make -j "${JOBS}" install DESTDIR="$(pwd)/Inst"
	[[ $? -ne 0 ]] && echo "Can't install!" && exit -1
	fi

echo "pushing"
# Push local Valgrind installtion to the phone
	if [[ $(adb shell ls -ld /data/local/Inst/bin/valgrind) = *"No such file or directory"* ]];
	then
	adb root
	adb remount
	adb shell "[ ! -d /data/local/Inst ] && mkdir /data/local/Inst"
	adb push Inst /
	adb shell "ls -l /data/local/Inst"

# Ensure Valgrind on the phone is running
	adb shell "/data/local/Inst/bin/valgrind --version"

# Add Valgrind executable to PATH (this might fail)
	adb shell "export PATH=$PATH:/data/local/Inst/bin/"
	fi

	if [ $RUN_HELLO_JNI_THROUGH_VALGRIND = true ]; then
	PACKAGE="com.example.hellojni"

# The location of the Hello JNI sample application
	HELLO_JNI_PATH="$ANDROID_NDK_HOME/samples/hello-jni"

	pushd "$HELLO_JNI_PATH" 

# Update build target to the desired Android SDK version
	ANDROID_PROJECT_TARGET="android-18"
	android update project --target "$ANDROID_PROJECT_TARGET" --path . --name hello-jni --subprojects

# Enable Android NDK build with Ant
	echo '<?xml version="1.0" encoding="utf-8"?>

	<project name="HelloJni" basedir="." default="debug">

	<target name="-pre-build">
	<exec executable="${ndk.dir}/ndk-build" failonerror="true"/>
	</target>

	<target name="clean" depends="android_rules.clean">
	<exec executable="${ndk.dir}/ndk-build" failonerror="true">
	<arg value="clean"/>
	</exec>
	</target> 

	</project>
	' > "custom_rules.xml"

# Set NDK HOME for Ant (only if not already set)
	if ! grep -P -q "ndk.dir=.+" "local.properties" ; then
	echo -e "\nndk.dir=$ANDROID_NDK_HOME" >> "local.properties"
	fi

# Fix for Java 8 warning (warning: [options] source value 1.5 is obsolete and will be removed in a future release)
	echo "java.compilerargs=-Xlint:-options" >> "ant.properties"

# Workaround INSTALL_PARSE_FAILED_INCONSISTENT_CERTIFICATES error
	adb uninstall "$PACKAGE"

# Build Hello JNI project in debug mode and install it on the device
	ant clean && ant debug && ant installd

	popd

	cd ..  

# Start HelloJNI app 
	adb shell am start -a android.intent.action.MAIN -n $PACKAGE/.HelloJni

# Make the script executable
	chmod a+x bootstrap_valgrind.sh

# Run application through Valgrind on the phone
	/usr/bin/env bash bootstrap_valgrind.sh

	adb shell ls -lR "/sdcard/*grind*"
	adb shell ls -lR "/storage/sdcard0/*grind*"
	adb shell ls -lR "/storage/sdcard1/*grind*"
	fi

	exit 0 
