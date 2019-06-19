--
dofile("config.lua");

PROJ_DIR = path.getabsolute("..")
BUILD_DIR = path.join(PROJ_DIR, "build")

solution "Resacred"
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
			"Symbols",
			"FullSymbols"
		}
		defines {
			"DEBUG",
			"CONF_DEBUG"
		}
	
	configuration {"Release"}
		targetsuffix "_release"
		flags {
			"Optimize",
			"NoBufferSecurityCheck"
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
		"EnableMinimalRebuild",
		"StaticRuntime",
	}
	
	defines {
        --"LOGGER_DONT_WRITE",
        --"SACRED_LOG_MEM_ALLOC",
		"RS_MATH_OPERATORS",
		"CONF_ENABLE_UI",
		"_ITERATOR_DEBUG_LEVEL=0"
    }
	
	-- disable exception related warnings
	buildoptions{ "/wd4577", "/wd4530" }
	

project "Resacred_client"
	kind "WindowedApp" --TODO: change to console

	configuration {}
	
	files {
		"src/**.h",
		"src/**.cpp",
	}
	
	includedirs {
		SDL2_include,
		ZLIB_include,
		bgfx_include
	}
	
	links {
		"gdi32",
		"glu32",
		"opengl32",
		SDL2_lib,
		ZLIB_lib,
		bgfx_libs
	}