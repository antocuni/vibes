#!/bin/bash
set -e

# Minimal Android build script - no Gradle, no Android Studio
# Compiles against API 23, targets modern Android devices

ANDROID_JAR="/usr/lib/android-sdk/platforms/android-23/android.jar"
AAPT2="/usr/lib/android-sdk/build-tools/debian/aapt2"
DX="/usr/lib/android-sdk/build-tools/debian/dx"
ZIPALIGN="/usr/lib/android-sdk/build-tools/debian/zipalign"
APKSIGNER="/usr/lib/android-sdk/build-tools/debian/apksigner"

SRC_DIR="app/src/main"
BUILD_DIR="build"
GEN_DIR="$BUILD_DIR/gen"
OBJ_DIR="$BUILD_DIR/obj"
APK_DIR="$BUILD_DIR/apk"

echo "=== Cleaning ==="
rm -rf "$BUILD_DIR"
mkdir -p "$GEN_DIR" "$OBJ_DIR" "$APK_DIR"

echo "=== Compiling resources with aapt2 ==="
# Compile each resource file individually
mkdir -p "$BUILD_DIR/compiled"
for resfile in $(find "$SRC_DIR/res" -type f -name "*.xml" -o -name "*.png" -o -name "*.jpg"); do
    "$AAPT2" compile "$resfile" -o "$BUILD_DIR/compiled/" 2>&1
done

echo "=== Linking resources ==="
"$AAPT2" link \
    -o "$BUILD_DIR/resources.apk" \
    --manifest "$SRC_DIR/AndroidManifest.xml" \
    -I "$ANDROID_JAR" \
    --java "$GEN_DIR" \
    --auto-add-overlay \
    --min-sdk-version 23 \
    --target-sdk-version 34 \
    --version-code 1 \
    --version-name "1.0" \
    "$BUILD_DIR/compiled/"*.flat

echo "=== Compiling Java sources ==="
find "$SRC_DIR/java" "$GEN_DIR" -name "*.java" > "$BUILD_DIR/sources.txt"
javac \
    --release 8 \
    -classpath "$ANDROID_JAR" \
    -d "$OBJ_DIR" \
    @"$BUILD_DIR/sources.txt"

echo "=== Converting to DEX ==="
"$DX" --dex --output="$BUILD_DIR/classes.dex" "$OBJ_DIR"

echo "=== Building APK ==="
# Start with the resources APK
cp "$BUILD_DIR/resources.apk" "$BUILD_DIR/unsigned.apk"
# Add the DEX file
cd "$BUILD_DIR"
zip -j unsigned.apk classes.dex
cd ..

echo "=== Aligning APK ==="
"$ZIPALIGN" -f 4 "$BUILD_DIR/unsigned.apk" "$BUILD_DIR/aligned.apk"

echo "=== Signing APK ==="
# Generate a keystore if it doesn't exist
if [ ! -f "release.keystore" ]; then
    echo "Generating release keystore..."
    keytool -genkeypair \
        -keystore release.keystore \
        -alias release \
        -keyalg RSA \
        -keysize 2048 \
        -validity 10000 \
        -storepass android \
        -keypass android \
        -dname "CN=Reminders App, OU=Dev, O=Vibes, L=Unknown, ST=Unknown, C=US"
fi

"$APKSIGNER" sign \
    --ks release.keystore \
    --ks-key-alias release \
    --ks-pass pass:android \
    --key-pass pass:android \
    --out "$BUILD_DIR/reminders.apk" \
    "$BUILD_DIR/aligned.apk"

echo ""
echo "=== Build complete ==="
ls -lh "$BUILD_DIR/reminders.apk"
echo ""
echo "APK: $BUILD_DIR/reminders.apk"
