--
dofile("config.lua");

PROJ_DIR = path.getabsolute("..")
BUILD_DIR = path.join(PROJ_DIR, "build")

solution "resacred_solution"
	location "build"
	
	configurations {
		"Debug",
		"Release"
	}

	platforms {
		"x64"
	}
	
	language "C++"
	
	configuration {"Debug"}
		targetsuffix "_debug"
		flags {
			"Symbols"
		}
		defines {
			"DEBUG",
			"CONF_DEBUG"
		}
	
	configuration {"Release"}
		targetsuffix "_release"
		flags {
			"Optimize"
		}
		defines {
			"NDEBUG",
			"CONF_RELEASE",
		}
	
	configuration {}
	
	targetdir(BUILD_DIR)
	
	includedirs {
		"src"
	}
	
	links {
		"user32",
		"shell32",
		"winmm",
		"ole32",
		"oleaut32",
		"imm32",
		"version",
		"ws2_32",
		"advapi32"
	}
	
	flags {
		"NoExceptions",
		"NoRTTI",
		"EnableSSE",
		"EnableSSE2",
		"EnableAVX",
		"EnableAVX2",
	}
	
	defines {
        --"LOGGER_DONT_WRITE",
        --"SACRED_LOG_MEM_ALLOC",
		"RS_MATH_OPERATORS",
        "CONF_ENABLE_UI"
    }
	
	-- disable exception related warnings
	buildoptions{ "/wd4577", "/wd4530" }
	

project "resacred_client"
	kind "WindowedApp" --TODO: change to console

	configuration {}
	
	files {
		"src/**.h",
		"src/**.cpp",
	}
	
	includedirs {
		SDL2_include,
		ZLIB_include,
	}
	
	links {
		"gdi32",
		"glu32",
		"opengl32",
		SDL2_lib,
		ZLIB_lib,
	}