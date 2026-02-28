; ModuleID = 'zlang_program'
source_filename = "zlang_program"

define i64 @max(i64 %0, i64 %1) {
entry:
  %a = load i64, i64 %0, align 4
  %b = load i64, i64 %1, align 4
  %gttmp = icmp sgt i64 %a, %b
  br i1 %gttmp, label %then, label %else

then:                                             ; preds = %entry
  %a1 = load i64, i64 %0, align 4
  ret i64 %a1

merge:                                            ; No predecessors!
  ret i64 0

else:                                             ; preds = %entry
  %b2 = load i64, i64 %1, align 4
  ret i64 %b2
}
