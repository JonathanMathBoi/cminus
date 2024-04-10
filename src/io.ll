@output_format = private constant [4 x i8] c"%d\0A\00" ; "%d\n"

; external declaration of printf
declare i32 @printf(ptr %format, ...)

define void @output(i32 %value) {
entry:
    %0 = call i32 (ptr, ...) @printf(ptr @output_format, i32 %value)
    ret void
}

@input_format = private constant [3 x i8] c"%d\00"

; external declaration of scanf
declare i32 @scanf(ptr %format, ...)

define i32 @input() {
entry:
    %buffer = alloca i32
    %0 = call i32 (ptr, ...) @scanf(ptr @input_format, ptr %buffer)
    %value = load i32, ptr %buffer
    ret i32 %value
}
