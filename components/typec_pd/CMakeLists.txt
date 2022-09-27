set(include_dirs  include core core/modules core/vdm core/vdm/DisplayPort)
file(GLOB c_srcs
    "core/*.c"
    "core/modules/*.c"
    "core/vdm/*.c" 
    "core/vdm/DisplayPort/*.c")
idf_component_register(SRCS "${c_srcs}" "platform.c" "typec_pd_main_task.c"
                       INCLUDE_DIRS "${include_dirs}" 
                       REQUIRES )