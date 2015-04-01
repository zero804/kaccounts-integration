function(kaccounts_add_provider provider_in_file)
    get_filename_component(provider_filename ${provider_in_file} NAME_WE)
    set(provider_file ${CMAKE_CURRENT_BINARY_DIR}/${provider_filename}.provider)
    execute_process(COMMAND intltool-merge -x -u --no-translations ${provider_in_file} ${provider_file})
    install(FILES ${provider_file} DESTINATION ${ACCOUNTS_PROVIDERS_DIR})
endfunction()

function(kaccounts_add_service service_file_in)
    get_filename_component(service_filename ${service_file_in} NAME_WE)
    set(service_file ${CMAKE_CURRENT_BINARY_DIR}/${service_filename}.service)
    execute_process(COMMAND intltool-merge -x -u --no-translations ${service_file_in} ${service_file})
    install(FILES ${service_file} DESTINATION ${ACCOUNTS_SERVICES_DIR})
endfunction()
