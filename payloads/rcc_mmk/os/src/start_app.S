    .align 3
    .section .data
    .global _app_num
_app_num:
    .quad 13

    .global _app_datas
_app_datas:
    .quad app_0 # value defined by "quad" is always 64 bit.
    .quad app_1
    .quad app_2
    .quad app_3
    .quad app_4
    .quad app_5
    .quad app_6
    .quad app_7
    .quad app_8
    .quad app_9
    .quad app_10
    .quad app_11
    .quad app_12
    .quad app_end

    .global _app_names
_app_names:
    .string "user_shell"
    .string "initproc"
    .string "hello_world"
    .string "mmktest"
    .string "syscall"
    .string "spawn"
    .string "pipe"
    .string "int"
    .string "long"
    .string "double"
    .string "hanoi"
    .string "float"
    .string "looper"


    .global app_0
app_0:
    .incbin "../user/build/user_shell.elf"

    .global app_1
app_1:
    .incbin "../user/build/initproc.elf"

    .global app_2
app_2:
    .incbin "../user/build/hello_world.elf"

    .global app_3
app_3:
    .incbin "../user/build/mmktest.elf"

    .global app_4
app_4:
    .incbin "../user/build/syscall.elf"

    .global app_5
app_5:
    .incbin "../user/build/spawn.elf"

    .global app_6
app_6:
    .incbin "../user/build/pipe.elf"

    .global app_7
app_7:
    .incbin "../user/build/int.elf"

    .global app_8
app_8:
    .incbin "../user/build/long.elf"

    .global app_9
app_9:
    .incbin "../user/build/double.elf"

    .global app_10
app_10:
    .incbin "../user/build/hanoi.elf"   

    .global app_11
app_11:
    .incbin "../user/build/float.elf"    

    .global app_12
app_12:
    .incbin "../user/build/looper.elf"    


    .global app_end
app_end:
