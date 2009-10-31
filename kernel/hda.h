#ifndef QO3_KERNEL_HDA_H
#define QO3_KERNEL_HDA_H

enum hda_init_error_code {
	HDA_INIT_NOT_FOUND
};
struct hda_init_error {
	enum hda_init_error_code code;
};

/* return negative if failed */
struct pci_root;
int hda_init(struct pci_root *pci, struct hda_init_error *error);
void hda_dump(void);

#endif
