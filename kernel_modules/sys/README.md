# Documentation
* followed: https://wiki.tldp.org/lkmpg/en/content/ch06/2_6


# /proc vs /sys
* /proc contains information about running processes and other kernel status info
* /sys is aimed at holding system information
* however, stuff about the system that was stored in /proc remains there


# Basic usage and implementation
## Basic functions
```
/*
 *
 */
kobject_create_and_add();

/*
 *
 */
sysfs_create_group();

/*
 * Decrements refcount for object
 * If the refcount is 0, calls kobject_cleanup
 */
void kobject_put(struct kobject * kobj);
```

## Structures
* Documentation: https://elixir.bootlin.com/linux/latest/source/include/linux/kobject.h
* https://medium.com/powerof2/the-kernel-kobject-device-model-explained-89d02350fa03
* http://www.staroceans.org/kernel-and-driver/The%20Linux%20Kernel%20Driver%20Model.pdf
* https://lwn.net/Articles/51437/
* https://lwn.net/Articles/55847/

```
/* 
 * Since C does not allow inheritance, kobjects are used for structure 
 * embedding, in which we place a kobject in the structure of another  
 * object.
 */
struct kobject {
    const char *name; // name
	
    struct list_head entry; // list entry in subsystem's list of objects
	
    struct kobject *parent; // parent pointer, allowing objects to be arraged into hierarchies
	
    struct kset	*kset;
	
    struct kobj_type *ktype; // specific type
	
    struct kernfs_node *sd; /* sysfs directory entry */
	
    struct kref	kref; // reference count
    
    
    
    
    
    
    const char *name; // name
    // reference count
    struct kobject *parent; // parent pointer, allowing objects to be arraged into hierarchies
    struct kobj_type *ktype; // specific type
    // representation in sysfs 
};

/* 
 *
 */
struct ktype {
    
};

/* 
 * group of kobjects, which can be of different ktypes.
 * when you see a sysfs directory full of other directories, generally 
 * each of those directories corresponds to a kobject in the same kset
 *
 * type of the object that embeds a kobject. It controls what happens to 
 * the kobject when it is created and destroyed. So these kobjects
 * should be grouped together to be operated the same way.
 * It is treated the same way as a kobject, because it is a 
 * kobject itself.
 * 
 * It can be used to track all block devices or all PCI device drivers.
 * ksets are always represented as directory in sysfs. kobjects do not
 * necessarily show up in sysfs, but every kobject that is a member of a
 * kset is represented there

 * Every kset must belong to a subsystem
 */
struct kset {
    list_head list; // list of kobjects for this kset
    
    spinlock list_lock;
    
    kobj kobj;
    
    kobj_type *ktype;
};

/*
 * Examples: * block devices: block-subsys (/sys/block)
 *           * core device hierarchy: device_subsys (/sys/devices)
 */
struct subsystem {
    struct kset kset; // one can find each kset's containing subsystem from the kset's structure, but one cannot find the multiple ksets contained in a subsystem directly from the subsystem structure
    struct rw_semaphore rwsem; // serializes accesses to the kset's 
    
    internal linked list
};
```

## Example usage
```
static struct kobj_type ktype_block = {
    .release = disk_release,
    .sysfs_ops = &disk_sysfs_ops,
    .default_attrs = default_attrs,
};

/* declare block_subsys. */
static decl_subsys(block, &ktype_block, &block_hotplug_ops);
#define decl_subsys(_name,_type,_hotplug_ops) \
struct subsystem _name##_subsys = { \
    .kset = { \
    .kobj = { .name = __stringify(_name) }, \
    .ktype = _type, \
    .hotplug_ops =_hotplug_ops, \
    } \
}

subsystem_register(&block_subsys);



```