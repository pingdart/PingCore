// platforms/android/app/build.gradle.kts
plugins {
    id("com.android.application")
}

android {
    namespace = "dev.pingdart.sharedlogin"
    compileSdk = 34

    defaultConfig {
        applicationId = "dev.pingdart.sharedlogin"
        minSdk = 26
        targetSdk = 34
        versionCode = 1
        versionName = "1.0.0"

        ndk {
            abiFilters.addAll(listOf("arm64-v8a", "x86_64"))
        }

        externalNativeBuild {
            cmake {
                cppFlags.addAll(listOf("-std=c++20", "-fexceptions", "-frtti", "-fstack-protector-strong"))
                arguments.addAll(listOf(
                    "-DANDROID_STL=c++_shared",
                    "-DPINGDART_BUILD_MOBILE=ON"
                ))
            }
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
        debug {
            isDebuggable = true
            isJniDebuggable = true
        }
    }

    externalNativeBuild {
        cmake {
            path = file("../../../CMakeLists.txt")
            version = "3.26.0+"
        }
    }

    packaging {
        jniLibs {
            useLegacyPackaging = true
        }
    }
}

dependencies {
    implementation("androidx.core:core:1.12.0")
    implementation("androidx.games:games-activity:3.0.4")
}
