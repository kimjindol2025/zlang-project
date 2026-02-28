; ModuleID = 'zlang_program'
source_filename = "zlang_program"

define i64 @test_lexer() {
entry:
  %x = alloca i32, align 4
  store i64 42, ptr %x, align 4
  %y = alloca double, align 8
  store double 3.140000e+00, ptr %y, align 8
  %s = alloca ptr, align 8
  store [6 x i8] c"hello\00", ptr %s, align 1
  %b = alloca i1, align 1
  store i1 true, ptr %b, align 1
  ret i64 100
}
