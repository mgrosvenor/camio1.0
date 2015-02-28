#ifndef DAG71S_PHY_COMPONENT_H

#define DAG71S_PHY_COMPONENT_H



ComponentPtr dag71s_get_new_phy_component(DagCardPtr card);
int dag71s_phy_read(ComponentPtr component, int phy_reg);
void dag71s_phy_write(ComponentPtr compoennt, int phy_reg, uint32_t val);

#endif

