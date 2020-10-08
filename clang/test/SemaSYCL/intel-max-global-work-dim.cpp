// RUN: %clang_cc1 %s -fsyntax-only -fsycl -fsycl-is-device -Wno-sycl-2017-compat -triple spir64 -DTRIGGER_ERROR -verify
// RUN: %clang_cc1 %s -fsyntax-only -ast-dump -fsycl -fsycl-is-device -Wno-sycl-2017-compat -triple spir64 | FileCheck %s
// RUN: %clang_cc1 -fsycl -fsycl-is-host -Wno-sycl-2017-compat -fsyntax-only -verify %s

#ifndef __SYCL_DEVICE_ONLY__
struct FuncObj {
  [[intel::max_global_work_dim(1)]] // expected-no-diagnostics
  void
  operator()() const {}
};

template <typename name, typename Func>
void kernel(const Func &kernelFunc) {
  kernelFunc();
}

void foo() {
  kernel<class test_kernel1>(
      FuncObj());
}

#else // __SYCL_DEVICE_ONLY__

[[intel::max_global_work_dim(2)]] void func_do_not_ignore() {}

struct FuncObj {
  [[intel::max_global_work_dim(1)]] void operator()() const {}
};

struct Func {
  // expected-warning@+2 {{attribute 'intelfpga::max_global_work_dim' is deprecated}}
  // expected-note@+1 {{did you mean to use 'intel::max_global_work_dim' instead?}}
  [[intelfpga::max_global_work_dim(2)]] void operator()() const {}
};

struct TRIFuncObjGood1 {
  [[intel::max_global_work_dim(0)]]
  [[intel::max_work_group_size(1, 1, 1)]]
  [[cl::reqd_work_group_size(1, 1, 1)]] void
  operator()() const {}
};

struct TRIFuncObjGood2 {
  [[intel::max_global_work_dim(3)]]
  [[intel::max_work_group_size(8, 1, 1)]]
  [[cl::reqd_work_group_size(4, 1, 1)]] void
  operator()() const {}
};

#ifdef TRIGGER_ERROR
struct TRIFuncObjBad {
  [[intel::max_global_work_dim(0)]]
  [[intel::max_work_group_size(8, 8, 8)]] // expected-error{{'max_work_group_size' X-, Y- and Z- sizes must be 1 when 'max_global_work_dim' attribute is used with value 0}}
  [[cl::reqd_work_group_size(4, 4, 4)]]   // expected-error{{'reqd_work_group_size' X-, Y- and Z- sizes must be 1 when 'max_global_work_dim' attribute is used with value 0}}
  void
  operator()() const {}
};
#endif // TRIGGER_ERROR

template <typename name, typename Func>
__attribute__((sycl_kernel)) void kernel(const Func &kernelFunc) {
  kernelFunc();
}

int main() {
  // CHECK-LABEL: FunctionDecl {{.*}}test_kernel1
  // CHECK:       SYCLIntelMaxGlobalWorkDimAttr {{.*}} 1
  kernel<class test_kernel1>(
      FuncObj());

  // CHECK-LABEL: FunctionDecl {{.*}}test_kernel2
  // CHECK:       SYCLIntelMaxGlobalWorkDimAttr {{.*}} 2
  // expected-warning@+3 {{attribute 'intelfpga::max_global_work_dim' is deprecated}}
  // expected-note@+2 {{did you mean to use 'intel::max_global_work_dim' instead?}}
  kernel<class test_kernel2>(
      []() [[intelfpga::max_global_work_dim(2)]]{});

  // CHECK-LABEL: FunctionDecl {{.*}}test_kernel3
  // CHECK:       SYCLIntelMaxGlobalWorkDimAttr {{.*}}
  kernel<class test_kernel3>(
      []() { func_do_not_ignore(); });

  kernel<class test_kernel4>(
      TRIFuncObjGood1());
  // CHECK-LABEL: FunctionDecl {{.*}}test_kernel4
  // CHECK:       ReqdWorkGroupSizeAttr {{.*}} 1 1 1
  // CHECK:       SYCLIntelMaxWorkGroupSizeAttr {{.*}} 1 1 1
  // CHECK:       SYCLIntelMaxGlobalWorkDimAttr {{.*}} 0

  kernel<class test_kernel5>(
      TRIFuncObjGood2());
  // CHECK-LABEL: FunctionDecl {{.*}}test_kernel5
  // CHECK:       ReqdWorkGroupSizeAttr {{.*}} 1 1 4
  // CHECK:       SYCLIntelMaxWorkGroupSizeAttr {{.*}} 1 1 8
  // CHECK:       SYCLIntelMaxGlobalWorkDimAttr {{.*}} 3

#ifdef TRIGGER_ERROR
  [[intel::max_global_work_dim(1)]] int Var = 0; // expected-error{{'max_global_work_dim' attribute only applies to functions}}

  kernel<class test_kernel6>(
      []() [[intel::max_global_work_dim(-8)]]{}); // expected-error{{'max_global_work_dim' attribute requires a non-negative integral compile time constant expression}}

  kernel<class test_kernel7>(
      []() [[intel::max_global_work_dim(3),
             intel::max_global_work_dim(2)]]{}); // expected-warning{{attribute 'max_global_work_dim' is already applied with different parameters}}

  kernel<class test_kernel8>(
      TRIFuncObjBad());

  kernel<class test_kernel9>(
      []() [[intel::max_global_work_dim(4)]]{}); // expected-error{{The value of 'max_global_work_dim' attribute must be in range from 0 to 3}}

#endif // TRIGGER_ERROR
}
#endif // __SYCL_DEVICE_ONLY__
