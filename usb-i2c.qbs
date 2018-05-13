import qbs

CppApplication {
    name: "usb-i2c"

    type: ["application", "hex", "bin"]

    consoleApplication: true

    cpp.libraryPaths: ["libopencm3/lib"]

    cpp.staticLibraries: ["c", "gcc", "nosys", "opencm3_stm32f0"]

    cpp.defines: ["STM32F0"]

    cpp.driverFlags: ["-mthumb", "-mcpu=cortex-m0"]
    cpp.cFlags: ["-ggdb3", "-fno-common", "-ffunction-sections", "-fdata-sections"]
    cpp.cxxFlags: ["-ggdb3", "-fno-common", "-ffunction-sections", "-fdata-sections"]
    cpp.driverLinkerFlags: ["-flto", "-static", "-nostartfiles"]
    cpp.linkerFlags: ["--gc-sections"]

    cpp.optimization: "small"
    cpp.debugInformation: true

    cpp.cLanguageVersion: "c11"
    cpp.cxxLanguageVersion: "c++17"

    cpp.warningLevel: "all"
    cpp.treatWarningsAsErrors: true

    cpp.positionIndependentCode: false

    cpp.executableSuffix: ".elf"

    cpp.includePaths: [
        "libopencm3/include",
        "freertos/include",
        "application/include"
    ]

    Group {
        name: "Application sources"
        files: [
            "application/include/*.h",
            "application/src/*.c",
        ]
    }

    Group {
        name: "FreeRTOS sources"
        files: [
            "freertos/include/*.h",
            "freertos/src/*.c",
        ]
    }

    Group {
        name: "Linker script"
        files: ["stm32f042f4.ld"]
        fileTags: ["linkerscript"]
    }

    Rule {
        inputs: "application"

        Artifact {
            fileTags: ["hex"]
            filePath: product.name + ".hex"
        }

        prepare: {
            var objcopyPath = product.moduleProperty("cpp", "objcopyPath");
            var args = ["-O", "ihex", input.filePath, output.filePath];
            var cmd = new Command(objcopyPath, args);
            cmd.description = "creating " + output.fileName;
            return cmd;
        }
    }

    Rule {
        inputs: "application"

        Artifact {
            fileTags: ["bin"]
            filePath: product.name + ".bin"
        }

        prepare: {
            var objcopyPath = product.moduleProperty("cpp", "objcopyPath");
            var args = ["-O", "binary", input.filePath, output.filePath];
            var cmd = new Command(objcopyPath, args);
            cmd.description = "creating " + output.fileName;
            return cmd;
        }
    }
}
