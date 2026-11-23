include_guard(GLOBAL)

function(pingcore_add_web_seo target_name)
    cmake_parse_arguments(SEO
            ""
            "OUTPUT_DIRECTORY;SITEMAP_URL"
            "ROUTES"
            ${ARGN}
    )
    if (NOT TARGET ${target_name})
        message(FATAL_ERROR "pingcore_add_web_seo() target does not exist: ${target_name}")
    endif ()
    if (NOT SEO_OUTPUT_DIRECTORY)
        set(SEO_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/seo")
    endif ()
    set_property(TARGET ${target_name} APPEND PROPERTY
            PINGCORE_SEO_OUTPUT_DIRECTORY "${SEO_OUTPUT_DIRECTORY}"
    )
    file(MAKE_DIRECTORY "${SEO_OUTPUT_DIRECTORY}")
    if (NOT SEO_SITEMAP_URL)
        set(SEO_SITEMAP_URL "")
    endif ()
    file(WRITE "${SEO_OUTPUT_DIRECTORY}/robots.txt"
         "User-agent: *\nAllow: /\n")
    if (SEO_SITEMAP_URL)
        file(APPEND "${SEO_OUTPUT_DIRECTORY}/robots.txt"
             "Sitemap: ${SEO_SITEMAP_URL}\n")
    endif ()
    file(WRITE "${SEO_OUTPUT_DIRECTORY}/sitemap.xml"
         "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<urlset xmlns=\"http://www.sitemaps.org/schemas/sitemap/0.9\"></urlset>\n")
    add_custom_target(${target_name}_seo
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${SEO_OUTPUT_DIRECTORY}"
            COMMENT "PingCore SEO output ready at ${SEO_OUTPUT_DIRECTORY}"
            VERBATIM
    )
    add_dependencies(${target_name} ${target_name}_seo)
    set_property(TARGET ${target_name} PROPERTY PINGCORE_SEO_ROUTES "${SEO_ROUTES}")
    set_property(TARGET ${target_name} PROPERTY PINGCORE_SITEMAP_URL "${SEO_SITEMAP_URL}")
endfunction()

