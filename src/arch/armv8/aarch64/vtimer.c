#include <vtimer.h>
#include <vm.h>

void vtimer_init(struct vcpu* vcpu)
{

}

void vtimer_reset(struct vcpu* vcpu)
{
    vcpu->arch.vtimer.cntv_ctl_el0 = 0;
    vcpu->arch.vtimer.cntv_cval_el0 = 0;
    vcpu->arch.vtimer.cntvoff_el2 = 0;
}

void vtimer_save_state(struct vcpu* vcpu)
{
    vcpu->arch.vtimer.cntkctl_el1 = sysreg_cntkctl_el1_read();
    vcpu->arch.vtimer.cntv_ctl_el0 = sysreg_cntv_ctl_el0_read();
    vcpu->arch.vtimer.cntv_cval_el0 = sysreg_cntv_cval_el0_read();
}

void vtimer_restore_state(struct vcpu* vcpu)
{
    sysreg_cntkctl_el1_write(vcpu->arch.vtimer.cntkctl_el1);
    sysreg_cntv_ctl_el0_write(vcpu->arch.vtimer.cntv_ctl_el0);
    sysreg_cntv_cval_el0_write(vcpu->arch.vtimer.cntv_cval_el0);
    sysreg_cntvoff_el2_write(vcpu->arch.vtimer.cntvoff_el2);
}
