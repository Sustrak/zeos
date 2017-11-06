cmake_minimum_required(VERSION 3.8)
project(zeos)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        include/asm.h
        include/devices.h
        include/entry.h
        include/hardware.h
        include/interrupt.h
        include/io.h
        include/keyboard.h
        include/libc.h
        include/list.h
        include/mm.h
        include/mm_address.h
        include/sched.h
        include/segment.h
        include/stats.h
        include/system.h
        include/types.h
        include/utils.h
        include/zeos_interrupt.h
        include/zeos_mm.h
        bootsect.S
        build.c
        devices.c
        entry.S
        hardware.c
        interrupt.c
        io.c
        libauxjp.a
        libc.c
        libzeos.a
        list.c
        Makefile
        mm.c
        sched.c
        sys.c
        system.c
        system.lds
        user.c
        user.lds
        utils.c include/errno.h errno.c perror.c include/perror.h include/sys_codes.h include/stdlib.h stdlib.c include/zeosarg.h zeosarg.c include/zeosarg.h zeosarg.c)

include_directories(include)

add_executable(zeos ${SOURCE_FILES})