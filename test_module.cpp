#include <iostream>
#include <llvm-c/Core.h>

int main() {
    LLVMContextRef ctx = LLVMContextCreate();
    LLVMModuleRef mod = LLVMModuleCreateWithNameInContext("test", ctx);
    
    std::cerr << "[1] Module created: " << (void*)mod << std::endl;
    
    // Print before adding function
    char* ir1 = LLVMPrintModuleToString(mod);
    std::cerr << "[2] IR before: " << strlen(ir1) << " bytes\n" << ir1 << std::endl;
    LLVMDisposeMessage(ir1);
    
    // Add a function
    LLVMTypeRef func_type = LLVMFunctionType(LLVMInt64TypeInContext(ctx), nullptr, 0, 0);
    LLVMValueRef func = LLVMAddFunction(mod, "test_func", func_type);
    std::cerr << "[3] Function added: " << (void*)func << std::endl;
    
    // Add entry block and return
    LLVMBuilderRef builder = LLVMCreateBuilderInContext(ctx);
    LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(ctx, func, "entry");
    LLVMPositionBuilderAtEnd(builder, entry);
    LLVMBuildRet(builder, LLVMConstInt(LLVMInt64TypeInContext(ctx), 42, 1));
    LLVMDisposeBuilder(builder);
    
    // Print after adding function
    char* ir2 = LLVMPrintModuleToString(mod);
    std::cerr << "[4] IR after: " << strlen(ir2) << " bytes\n" << ir2 << std::endl;
    LLVMDisposeMessage(ir2);
    
    LLVMDisposeModule(mod);
    LLVMContextDispose(ctx);
    
    return 0;
}
