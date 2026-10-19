plugins {
    id("java")
    id("org.jetbrains.intellij.platform") version "2.19.0"
}

group = "com.keet"
version = "1.0.0"

repositories {
    mavenCentral()
    intellijPlatform {
        defaultRepositories()
    }
}

dependencies {
    intellijPlatform {
        clion("2024.1")
    }
}

intellijPlatform {
    pluginConfiguration {
        id = "com.keet.intellij"
        name = "KeetScript Support"
        version = "1.0.0"
        description = "Language support, syntax highlighting, autocompletion, and live diagnostics for KeetScript (.keet) Single-File Components in CLion and IntelliJ Platform IDEs."
        vendor {
            name = "Keet Framework"
        }
    }
}
