#ifndef _ASM_SIM_DMA_MAPPING_H
#define _ASM_SIM_DMA_MAPPING_H

static inline int
dma_supported(struct device *dev, u64 mask)
{
  BUG();
  return(0);
}

#endif /* _ASM_SIM_DMA_MAPPING_H */
