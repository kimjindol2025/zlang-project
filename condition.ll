; ModuleID = 'zlang_program'
source_filename = "zlang_program"

define i64 @max(i64 %0, i64 %1) {
entry:
  %a = load i64, i64 %0, align 4
  %b = load i64, i64 %1, align 4
  ret i64 0
}
