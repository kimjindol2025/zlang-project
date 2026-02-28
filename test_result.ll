; ModuleID = 'zlang_program'
source_filename = "zlang_program"

define i64 @divide_safe(i64 %0, i64 %1) {
entry:
  %b = load i64, i64 %1, align 4
  %eqtmp = icmp eq i64 %b, 0
  br i1 %eqtmp, label %then, label %merge

then:                                             ; preds = %entry
  br label %merge

merge:                                            ; preds = %then, %entry
  %a = load i64, i64 %0, align 4
  %b1 = load i64, i64 %1, align 4
  %cmp = fcmp oeq i64 %b1, i64 0
  %ext = zext i1 %cmp to i64
  %is_zero_check = icmp eq i64 %b1, i64 %ext
  %divtmp = sdiv i64 %a, %b1
  ret i64 %divtmp
}
