#ifndef DAG71S_IMPL_H

#define DAG71S_IMPL_H

void dag71s_mdio_write(DagCardPtr card, int mdio_reg, uint32_t val);

int dag71s_mdio_read(DagCardPtr card, int mdio_reg);

#endif



