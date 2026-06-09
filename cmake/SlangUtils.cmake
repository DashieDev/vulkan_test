find_program(SLANGC_EXECUTABLE NAMES slangc REQUIRED)

function(target_compile_slang_shaders target_name)
    set(SPV_TEMP_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated_shaders")
    file(MAKE_DIRECTORY "${SPV_TEMP_DIR}")

    set(spv_outputs "")
    foreach(input_shader ${ARGN})
        get_filename_component(shader_dir "${input_shader}" DIRECTORY)
        string(REGEX REPLACE "^src/?" "" shader_dir "${shader_dir}")
        
        get_filename_component(shader_name "${input_shader}" NAME_WLE)

        set(temp_output_dir "${SPV_TEMP_DIR}/${shader_dir}")
        file(MAKE_DIRECTORY "${temp_output_dir}")
        
        set(spv_output "${temp_output_dir}/${shader_name}.spv")

        # Compile the .slang file.
        add_custom_command(
            OUTPUT "${spv_output}"
            COMMAND "${SLANGC_EXECUTABLE}" 
                "${CMAKE_CURRENT_SOURCE_DIR}/${input_shader}" 
                -target spirv 
                -profile spirv_1_4 
                -fvk-use-entrypoint-name 
                -o "${spv_output}"
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${input_shader}"
            COMMENT "Compiling Slang shader: ${input_shader} -> ${spv_output}"
            VERBATIM
        )

        list(APPEND spv_outputs "${spv_output}")

        add_custom_command(
            TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory 
                "$<TARGET_FILE_DIR:${target_name}>/${shader_dir}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${spv_output}"
                "$<TARGET_FILE_DIR:${target_name}>/${shader_dir}/"
            COMMENT "Copying Compiled Shader ${shader_name}.spv to target directory ${shader_dir}/..."
            VERBATIM
        )
    endforeach()

    add_custom_target(${target_name}_SlangShaders DEPENDS ${spv_outputs})
    add_dependencies(${target_name} ${target_name}_SlangShaders)
endfunction()