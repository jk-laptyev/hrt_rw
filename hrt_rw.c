#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");

#define MS_TO_NS(x)     (x * 1E6L)

static struct hrtimer hr_timer;

static long restart = 5;
static unsigned long delay_in_ms = 200L;

enum hrtimer_restart my_hrtimer_callback( struct hrtimer *timer)
{
	pr_info( "my_hrtimer_callback called (%lld).\n", ktime_to_ms(timer->base->get_time()));
	if (restart--) {
		hrtimer_forward_now(timer, ns_to_ktime(MS_TO_NS(delay_in_ms)));
		return HRTIMER_RESTART;
	}

	return HRTIMER_NORESTART;
}

/* the /proc function: allocate everything to allow concurrency */
static int hr_timer_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%ld (%lu)\n", restart, delay_in_ms);
    return 0;
}

static int hr_timer_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, hr_timer_proc_show, NULL);
}

static const struct file_operations hr_timer_proc_fops = {
	.open		= hr_timer_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static ssize_t hrt_restart_write( struct file *filp, const char __user *buff, unsigned long len, loff_t *offp )
{
	if (len > 64) {
		printk(KERN_INFO "hrt_restart_write: buffer is to large!\n");
		return -ENOSPC;
	}

	if (kstrtol_from_user( buff, len , 10, &restart)) {
		return -EFAULT;
	}
	return len;
}

struct file_operations hrt_restart_proc_fops = {
	.write = hrt_restart_write
};

static ssize_t hrt_delay_write( struct file *filp, const char __user *buff, unsigned long len, loff_t *offp )
{
	if (len > 64) {
		printk(KERN_INFO "hrt_delay_write: buffer is to large!\n");
		return -ENOSPC;
	}

	if (kstrtoul_from_user( buff, len , 10, &delay_in_ms)) {
		return -EFAULT;
	}
	return len;
}

static const struct file_operations hrt_delay_proc_fops = {
//	.open		= hrt_delay_proc_open,
	.write		= hrt_delay_write,
//	.release	= single_release,
};

int init_module(void)
{
	ktime_t ktime;

	pr_info("HR Timer module installing\n");

	proc_create("hrtimer", 0, NULL, &hr_timer_proc_fops);
	proc_create("hrt_restart", 0600, NULL, &hrt_restart_proc_fops);
	proc_create("hrt_delay", 0600, NULL, &hrt_delay_proc_fops);

	ktime = ktime_set(0, MS_TO_NS(delay_in_ms));

	hrtimer_init(&hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);

	hr_timer.function = &my_hrtimer_callback;

	pr_info( "Starting timer to fire in %lld ms (%ld)\n", ktime_to_ms(hr_timer.base->get_time()) + delay_in_ms, jiffies);

	hrtimer_start( &hr_timer, ktime, HRTIMER_MODE_REL );

	return 0;
}

void cleanup_module( void )
{
	int ret;

	remove_proc_entry("hrt_restart", NULL);
	remove_proc_entry("hrt_delay", NULL);
	remove_proc_entry("hrtimer", NULL);

	ret = hrtimer_cancel( &hr_timer );
	if (ret)
		pr_info("The timer was still in use...\n");

	pr_info("HR Timer module uninstalling\n");

	return;
}
