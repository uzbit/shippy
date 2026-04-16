plugins {
    id("com.android.application")
}

android {
    namespace = "com.uzbit.shippy"
    compileSdk = 36
    ndkVersion = "29.0.13599879"

    defaultConfig {
        applicationId = "com.uzbit.shippy"
        minSdk = 24
        targetSdk = 36
        versionCode = 1
        versionName = "1.0"

        externalNativeBuild {
            cmake {
                arguments += listOf(
                    "-DANDROID_STL=c++_shared"
                )
                abiFilters += "arm64-v8a"
            }
        }
    }

    externalNativeBuild {
        cmake {
            path = file("../../CMakeLists.txt")
            version = "3.22.1"
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    sourceSets {
        getByName("main") {
            assets.srcDirs("src/main/assets")
            // SDL3 Java bridge classes from locally fetched source
            java.srcDirs(
                "src/main/java",
                "../SDL3-src/android-project/app/src/main/java"
            )
        }
    }
}
