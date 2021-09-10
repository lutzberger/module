#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include "mmap.h"
#include "net.h"
#include <linux/moduleparam.h>

static DEFINE_MUTEX(mmap_device_mutex);



static int size = 65536; // default

static int size_op_write_handler(const char * val, const struct kernel_param *kp)
{
	printk("size_op_write_handler\n");
	size=simple_strtol(val,NULL,10);

	return size;
}
static int notify_param(const char *val, const struct kernel_param *kp)
{
		printk("notify_param\n");
        int res = param_set_int(val, kp); // Use helper for write variable
        if(res==0) {
                printk(KERN_INFO "New value of size = %d\n", size);
                return 0;
        }
		else
                printk(KERN_WARNING "cannot parse %s\n",val);
        return -1;
}

static int size_op_read_handler(char *buffer, const struct kernel_param *kp)
{
	printk("size_op_read_handler %d\n",size);
	snprintf(buffer,10,"%d",size);

	return strlen(buffer);
}

static const struct kernel_param_ops size_op_ops = {
	.set = &notify_param,
	.get = &size_op_read_handler
};

module_param_cb(size, &size_op_ops, &size, S_IRUGO|S_IWUSR);

int mmap_open(struct inode *inode, struct file *filp)
{
	struct mmap_info *info = NULL;

	if (!mutex_trylock(&mmap_device_mutex)) {
		printk(KERN_WARNING
		       "Another process is accessing the device\n");
		return -EBUSY;
	}

	info = kmalloc(sizeof(struct mmap_info), GFP_KERNEL);
	info->data = (char *)get_zeroed_page(GFP_KERNEL);
	memcpy(info->data, "Hello from kernel this is file: ", 32);
	memcpy(info->data + 32, filp->f_path.dentry->d_name.name,
	       strlen(filp->f_path.dentry->d_name.name));
	/* assign this info struct to the file */
	filp->private_data = info;    
    return 0;
}

void mmap_open1(struct vm_area_struct *vma)
{
	struct mmap_info *info = (struct mmap_info *)vma->vm_private_data;
	info->reference++;
}



void mmap_close(struct vm_area_struct *vma)
{
	struct mmap_info *info = (struct mmap_info *)vma->vm_private_data;
    printk("after:%s\n",info->data);
	sendpacket(info->data,strlen(info->data));
	info->reference--;
}

#if LINUX_VERSION_CODE > KERNEL_VERSION(4,1,39)
static unsigned int mmap_fault( struct vm_fault *vmf)
{
	struct page *page;
	struct mmap_info *info;
	struct vm_area_struct *vma=vmf->vma;
    printk("mmap_fault gfp_mask:%x pgoff:%ld\n",vmf->gfp_mask,vmf->pgoff);
#else

static int mmap_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	struct page *page;
	struct mmap_info *info;
    printk("mmap_fault flags:%x pgoff:%ld\n",vmf->flags,vmf->pgoff);
#endif
	info = (struct mmap_info *)vma->vm_private_data;
	if (!info->data) {
		printk("No data\n");
		return 0;
	}

	page = virt_to_page(info->data);

	get_page(page);
	vmf->page = page;

	return 0;
}

struct vm_operations_struct mmap_vm_ops = {
	.open = mmap_open1,
	.close = mmap_close,
	.fault = mmap_fault,
};




int mmapfop_close(struct inode *inode, struct file *filp)
{

	struct mmap_info *info = filp->private_data;
    printk("mmapfop_close\n");

	free_page((unsigned long)info->data);
	kfree(info);
	filp->private_data = NULL;

	mutex_unlock(&mmap_device_mutex);

	return 0;
}




int memory_map (struct file * f, struct vm_area_struct * vma)
{
    vma->vm_ops = &mmap_vm_ops;
	vma->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP;
	vma->vm_private_data = f->private_data;
	mmap_open1(vma);    
    return 0;
}
int mmap_ops_init(void)
{
        mutex_init(&mmap_device_mutex);
        return 0;
}