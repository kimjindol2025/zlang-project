; ModuleID = 'zlang_program'
source_filename = "zlang_program"

define i64 @divide() {
entry:
  %a = alloca i64, align 8
  store i64 10, ptr %a, align 4
  %b = alloca i64, align 8
  store i64 0, ptr %b, align 4
  %a1 = load i64, ptr %a, align 4
  %b2 = load i64, ptr %b, align 4
  %cmp = fcmp oeq i64 %b2, i64 0
  %ext = zext i1 %cmp to i64
  %is_zero_check = icmp eq i64 %b2, i64 %ext
  %divtmp = sdiv i64 %a1, %b2
  ret i64 %divtmp
}
