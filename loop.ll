; ModuleID = 'zlang_program'
source_filename = "zlang_program"

define i64 @loop_test() {
entry:
  %i = alloca i64, align 8
  store i64 0, ptr %i, align 4
  br label %while.cond

while.cond:                                       ; preds = %entry
  %i1 = load i64, ptr %i, align 4
  %i2 = load i64, ptr %i, align 4
  ret i64 %i2

while.body:                                       ; No predecessors!

while.end:                                        ; No predecessors!
}
