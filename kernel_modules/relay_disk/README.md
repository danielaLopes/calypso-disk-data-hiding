# API
## struct bio
* bio_alloc(gfp_t gfp_mask,         unsigned int nr_iovecs);
 * allocates a new bio, memory pool backed, with nr_iovecs bvecs. If gfp_mask contains __GFP_WAIT, the allocation is guaranteed to succeed
 * args:
    * gfp_mask: allocation mask to use
    * nr_iovecs: number of iovecs
* return: 
    * success: pointer to new bio
    * failure: NULL

* alloc_page(unsigned int gfp_mask);
    * args:
        * gfp_mask: get free pages flag; tells the allocator how it must behave, if GFP_WAIT is not set, the allocator will not block and instead retur null if memory is tight

* bio_add_page(struct bio *bio, 
                struct page *page,
                unsigned int len,
                unsigned int offset);
    * attempts to add a page to the bio_vec maplist. Only fails if either bio_bi_vcnt == bio->bi_max_vecs or it's a cloned bio
    * args:
        * bio is the destination bio
        * page is the page to add
        * len is the vec entry length
        * offset is the vec entry offset

* submit_bio_wait(struct bio *bio);
    * submits a bio and waits until it completes

* bio_put(struct bio *bio);
    * releases a reference to a bio, so the last put of a bio will free it

* __free_page(struct page *page)


## struct block_device
* blkdev_get_by_path()

* blkdev_put()