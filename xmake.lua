-- change to your paths, obviously
local skyrim_se_dir = "C:/SteamLibrary/steamapps/common/Skyrim Special Edition"
local sksevr_source_dir = "C:/Code/sksevr_2_00_12/Data/Scripts/Source"
local modlist_mods_dirs = {"C:/Games/NewSkyrimVR/mods", "C:/Games/MySkyrimVR/modlist/mods"}
local ffdec_cli_jar = "C:/Users/nadako/scoop/apps/ffdec/current/ffdec-cli.jar"

local version = "0.4.1"
local name = "SurvivalModeVR"

set_project(name)
set_version(version)

set_arch("x64")
set_languages("c++23")
set_warnings("allextra")

add_rules("mode.debug", "mode.releasedbg")
set_defaultmode("releasedbg")
set_policy("build.optimization.lto", true) -- don't lose optimizations just because stuff is in different files

add_rules("plugin.compile_commands.autoupdate")

set_config("skyrim_se", false)
set_config("skyrim_ae", false)
set_config("skyrim_vr", true)
set_config("rex_ini", true)
set_config("skse_xbyak", true)
includes("libs/CommonLibSSE-NG")

target("patch-hud")
	set_kind("phony")
	set_default(false)
	on_build(function(target)
	    local misc_dir = path.join(os.projectdir(), "misc")
	    local tmp_dir = path.join(path.absolute(target:autogendir()), target:name())
	    local out_swf = path.join(os.projectdir(), "data", "Interface", "vr", "vr_hmd_info.swf")
	    os.mkdir(tmp_dir)
	    os.mkdir(path.directory(out_swf))

	    local src_xml = path.join(tmp_dir, "hudmenu.xml")
	    local dst_xml = path.join(tmp_dir, "vr_hmd_info.xml")
	    local out_xml = path.join(tmp_dir, "vr_hmd_info_out.xml")

	    local patched_swf = path.join(tmp_dir, "vr_hmd_info_patched.swf")
	    os.execv("java", {"-jar", ffdec_cli_jar, "-onerror", "abort", "-swf2xml", path.join(misc_dir, "hud-ae", "hudmenu.swf"), src_xml})
	    os.execv("java", {"-jar", ffdec_cli_jar, "-onerror", "abort", "-swf2xml", path.join(misc_dir, "hud-vr", "vr_hmd_info.swf"), dst_xml})
	    os.execv("python", {path.join(misc_dir, "patch_swf.py"), src_xml, dst_xml, out_xml,
	        "776:1158",
	        "784:1169",
	        "792:1176",
	        "521:906",
	        "+__Packages.Components.BlinkOnDemandPenaltyMeter",
	        "+__Packages.Components.BlinkOnEmptyPenaltyMeter",
	        "*CompassShoutMeterHolder=0.8",
	    })
	    os.execv("java", {"-jar", ffdec_cli_jar, "-onerror", "abort", "-xml2swf", out_xml, patched_swf})

	    local scripts_src_dir = path.join(misc_dir, "hud")
	    local scripts_dir = path.join(tmp_dir, "scripts")
	    os.rm(scripts_dir)
	    for _, file in ipairs(os.files(path.join(scripts_src_dir, "**.as"))) do
	        local target_file = path.join(scripts_dir, "__Packages", path.relative(file, scripts_src_dir))
	        os.mkdir(path.directory(target_file))
	        os.cp(file, target_file)
	    end
	    os.execv("java", {"-jar", ffdec_cli_jar, "-config", "autoDeobfuscate=false,decompile=false", "-onerror", "abort", "-importScript", patched_swf, out_swf, scripts_dir})
	    print("Built " .. out_swf)
	end)
target_end()

target(name)
	set_kind("shared")
	add_rules("commonlibsse-ng.plugin", {
	    name = name,
	    author = "nadako",
	    description = "Survival Mode VR Engine Patch"
	})
	set_pcxxheader("src/pch.h")
	add_includedirs("mod_api")
	add_files("src/**.cpp")

	before_build(function(target)
	    local scripts_dir = path.join(os.projectdir(), "data", "Scripts")
	    local mod_source_dir = path.join(scripts_dir, "Source")

	    os.execv(path.join(skyrim_se_dir, "Papyrus Compiler", "PapyrusCompiler.exe"), {
	        mod_source_dir,
	        "-all",
	        "-f=TESV_Papyrus_Flags.flg",
	        "-i=" .. table.concat({mod_source_dir, sksevr_source_dir, path.join(skyrim_se_dir, "Data", "Source", "Scripts")}, ";"),
	        "-o=" .. scripts_dir,
	    })
	end)

	after_build(function(target)
        import("utils.archive")

        local source_data_dir = path.join(os.projectdir(), "data")
        local source_data_files = {}
        for _, file in ipairs(os.files(path.join(source_data_dir, "**"))) do
            if not ("/" .. path.relative(file, source_data_dir)):find("[/\\]%.") then
                table.insert(source_data_files, file)
            end
        end

        local dist_dir = path.absolute(vformat("$(builddir)/dist"))
        local dist_mod_dir = path.join(dist_dir, name)
        local dist_archive = path.join(dist_dir, name .. "-" .. target:version() .. ".zip")
        os.rm(dist_dir)

        local target_dirs = {dist_mod_dir}
        for _, modlist_mods_dir in ipairs(modlist_mods_dirs) do
            table.insert(target_dirs, path.join(modlist_mods_dir, name))
        end
        for _, target_dir in ipairs(target_dirs) do
            local target_plugin_dir = path.join(target_dir, "SKSE", "Plugins")
            os.mkdir(target_plugin_dir)
            os.cp(target:targetfile(), target_plugin_dir)
            os.cp(target:symbolfile(), target_plugin_dir)

            for _, file in ipairs(source_data_files) do
                local target_file = path.join(target_dir, path.relative(file, source_data_dir))
                os.mkdir(path.directory(target_file))
                os.cp(file, target_file)
            end
            print("Deployed into " .. target_dir)
        end

        archive.archive(dist_archive, "*", {curdir = dist_mod_dir})
        print("Packaged " .. dist_archive)
	end)
target_end()
