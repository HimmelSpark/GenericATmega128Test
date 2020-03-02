/*
 * nav.h
 *
 * Created: 05.12.2019 13:26:44
 *  Author: Vsevolod
 */ 


#ifndef NAV_H_
#define NAV_H_

typedef struct{
	float lat;
	float lon;
	} NAV_WP;

// ��� ���������, ������������ �����������
typedef struct{
	uint8_t		id;		// ������������� ��������
	
	uint8_t		wp_n;	// ����� � ��������
	uint8_t		wp_tgt;	// ������� �����
	
	NAV_WP		*wps;	// ������ �����
	
	uint16_t	brg;	// ������ �� ��������� �����
	int16_t		dpsi;	// ���������������
	float		dst;	// ���������� �� �����
	} NAV_ROUTE;

// ��� ��������� ��� ������������� �������������
typedef struct{
	uint8_t		rte_id;	// ������������� ��������
	uint8_t		wp_n;		// ����� � ��������
	uint8_t		wp_tgt;	// ������� �����
	uint16_t	brg;	// ������ �� ��������� �����
	int16_t		dpsi;	// ���������������
	float		dst;	// ���������� �� �����
	} NAV_ROUTE_PROGRESS;

// �������������, � �.�. ��������
void nav_init(void);

// ��������/��������� �����������
void nav_autopilot_engage(void);
void nav_autopilot_diseng(void);

// ����������� �� ��������
void nav_autopilot(void);

// ������������ �� ��������� �����
uint8_t nav_route_next_wp(void);

// ToDo: ��������� ���������� �� �����������
void nav_route_get_progress(NAV_ROUTE_PROGRESS *progress);

// ������ �� ����� 0 �� ����� 1; �������
uint16_t nav_brg_p2p(float lat0, float lon0, float lat1, float lon1);

// ���������� �� ����� 0 �� ����� 1; �����
float nav_dst_p2p(float lat0, float lon0, float lat1, float lon1);

// ������ ������� �����
void nav_set_tgt_wp(float lat, float lon);

// �������� ������� �����
void nav_get_tgt_wp(float *lat, float *lon);

// ������ �� ������� ����� (�� ������ nav_brg_p2p)
uint16_t nav_brg2tgt(float lat, float lon);

// ���������� �� ������� ����� (�� ������ nav_dst_p2p)
float nav_dst2tgt(float lat, float lon);

// �������������: ��������������� ����� ������������ ������� �� ������� �����
// (�� ������ ���������� � ���������� ������������ � �������� ����������� ����)
uint16_t nav_dpsi2tgt(float lat, float lon, uint16_t crs);


// ������������ ��� ���������� ��������� � ����������� ������� �����
// (���������� ��������� ��� ������� ������ ����������)
#define K_l			3597865.4	// dl [���] * K_l = dz
#define K_phi		6372518.7	// dphi [���] * K_phi = dx
#define K_l__K_phi	0.5645908	// K_l/K_phi

// ������������ �����������
#define NAV_AUTO_PERIOD			1000	// ��; ������������� ������ ��������� �����������

#define NAV_AUTO_V_CRUIZE		0.7		// �/�; �������� ���������� �� ��������
#define NAV_AUTO_V_APPROACH		0.3		// �/�; �������� ������� � �����

#define NAV_AUTO_D_APPROACH		15.0	// �; ����������� ������� � �����
#define NAV_AUTO_D_CAPTURE		6.0		// �; ����������� ������� �����


#define NAV_AUTO_dPSI_MIN		-45
#define NAV_AUTO_dPSI_MAX		45
#define NAV_AUTO_dPSI_LIM		45	// �; ����������� ��������������� �� �����
#define NAV_OMEGA_OBJ_MAX		20	// �/�; ��������������� �������� ������� ��������
#define NAV_AUTO_K_dPSI			((float)NAV_OMEGA_OBJ_MAX/(float)NAV_AUTO_dPSI_LIM)	// ����������� ������� �������� �� ���������������
																		// Omega_tgt = NAV_AUTO_K_dPSI * dpsi


#endif /* NAV_H_ */