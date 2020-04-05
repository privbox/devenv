enable_language(C ASM)

add_executable(mtest src/main.c src/kerncall.c)
target_include_directories(mtest PRIVATE ./include/)
target_compile_definitions(mtest PUBLIC SYSCONFDIR="${CMAKE_INSTALL_SYSCONFDIR}")
install(TARGETS mtest DESTINATION bin)
