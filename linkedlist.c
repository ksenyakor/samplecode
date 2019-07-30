#include <stdlib.h>
#include <string.h>
#include <linkedlist.h>
#include <cmsis_os.h>
#include <kbd.h>
#include "lcd.h"
//#include "menu.h"
#include <kbd_input.h>
#include "stm32f4xx_hal.h"
#include "menu.h"
#include "reports.h"
#include "memory_i.h"
#include "memory_eeprom.h"
#include "tprinter.h"
#include "weight.h"
#include "misc.h"

extern KassaContext context;
extern Session MySession;
extern RTC_HandleTypeDef hrtc;

char paramitemstr[MAXSTRLEN];

char temp;
extern uint8_t kbd_input_cur_mode;
extern bool dep_input;
static void cutstr(char *str);
static int CheckDateTime(RTC_DateTypeDef sDate, RTC_TimeTypeDef sTime);

char str_no_cash_type[50]= "����������� ������ ������";

char strcode[MAXSTRLEN];
extern char paramitemstr[MAXSTRLEN];

//char *paramitemstr;
uint8_t newvaltick;
int paramitemcount_out; //���������� ����������  �������� � ������� ������
uint8_t first_part = 1;

#define IsOptionsMenu(x) (((x==kkm_opt)||(x==print_opt)||(x==moneybox_opt)) ? (1):(0))

kbd_inputPrototype inputFuncProto[] = { kbd_input_str, kbd_input_int, kbd_input_int, kbd_input_int,
		kbd_input_double, kbd_input_ticks, kbd_input_list,kbd_input_str };

kbd_noeditPrototype input_noedit_FuncProto[] = { kbd_noedit_str, kbd_noedit_int,kbd_noedit_int,kbd_noedit_int,
		kbd_noedit_double, kbd_noedit_ticks, kbd_noedit_list };

param_tick_item user_rights[7] = { { "A", 0 }, { "B", 0 }, { "C", 0 },{ "D", 0 }, {
		"E", 0 }, { "F", 0 },  { "G", 0 }}; // ����� ������������ �������� �� ���������
param_tick_item good_drob[1] = { { "��", 1 } }; //����� "�������" � ������� �������
param_tick_item discount_type[2] = { { "������", 1 }, { "�������", 0 } }; //������/�������

/*
 *�������  ��������� ��������� NodeItem
 */
NodeItem* createNodeItem(uint8_t type, char* title, param_table_item *value) {
	NodeItem *tmp = (NodeItem*) osAllocMem(sizeof(NodeItem));
	if (tmp == NULL) {
		exit(1);
	}
	tmp->type = type;
	tmp->title = strdup(title);

	if (value == NULL) {
		} else {
		tmp->value = value;
	}
	return tmp;
}


/*
 *�������  ��������� ��������� DblLinkedList
 *  editable - ������������� ������?
 */
DblLinkedList* createDblLinkedList(uint8_t editable) {
	DblLinkedList *tmp = (DblLinkedList*) osAllocMem(sizeof(DblLinkedList));
	tmp->size = 0;
	tmp->editable=editable;
	tmp->head = tmp->tail = NULL;

	return tmp;
}

/*
 *�������  ��������� ��������� DblLinkedList
 */
void deleteDblLinkedList(DblLinkedList **list) {
	Node *tmp = (*list)->head;
	Node *next = NULL;
	while (tmp) {
		next = tmp->next;
		osFreeMem(tmp->value->title);
		osFreeMem(tmp->value->value);
		osFreeMem(tmp->value);
		osFreeMem(tmp);
		tmp = next;
	}
	osFreeMem(*list);
	(*list) = NULL;
}

/*
 *������� � ������ DblLinkedList
 */
void pushFront(DblLinkedList *list, NodeItem *data) {
	Node *tmp = (Node*) osAllocMem(sizeof(Node));
	if (tmp == NULL) {
		exit(1);
	}
	tmp->value = data;
	tmp->next = list->head;
	tmp->prev = NULL;
	if (list->head) {
		list->head->prev = tmp;
	}
	list->head = tmp;

	if (list->tail == NULL) {
		list->tail = tmp;
	}
	list->size++;
}

/*
 *�������� �� ������ DblLinkedList
 */
NodeItem* popFront(DblLinkedList *list) {
	Node *prev;
	NodeItem *tmp;
	if (list->head == NULL) {
		exit(2);
	}

	prev = list->head;
	list->head = list->head->next;
	if (list->head) {
		list->head->prev = NULL;
	}
	if (prev == list->tail) {
		list->tail = NULL;
	}
	tmp = prev->value;
	osFreeMem(prev);

	list->size--;
	return tmp;
}

/*
 *������� � ����� DblLinkedList
 */
void pushBack(DblLinkedList *list, NodeItem *value) {
	Node *tmp = (Node*) osAllocMem(sizeof(Node));
	if (tmp == NULL) {
		exit(3);
	}
	tmp->value = value;
	tmp->next = NULL;
	tmp->prev = list->tail;
	if (list->tail) {
		list->tail->next = tmp;
	}
	list->tail = tmp;

	if (list->head == NULL) {
		list->head = tmp;
	}
	list->size++;
}
/*
 *�������� �� ����� DblLinkedList
 */
NodeItem* popBack(DblLinkedList *list) {
	Node *next;
	NodeItem *tmp;
	if (list->tail == NULL) {
		exit(4);
	}

	next = list->tail;
	list->tail = list->tail->prev;
	if (list->tail) {
		list->tail->next = NULL;
	}
	if (next == list->head) {
		list->head = NULL;
	}
	tmp = next->value;
	osFreeMem(next);

	list->size--;
	return tmp;
}

/*
 * ��������� n-�� ��������
 */
Node* getNthq(DblLinkedList *list, size_t index) {
	Node *tmp = NULL;
	size_t i;

	if (index < list->size / 2) {
		i = 0;
		tmp = list->head;
		while (tmp && i < index) {
			tmp = tmp->next;
			i++;
		}
	} else {
		i = list->size - 1;
		tmp = list->tail;
		while (tmp && i > index) {
			tmp = tmp->prev;
			i--;
		}
	}

	return tmp;
}


/*
 * �������� ����
 */
NodeItem* deleteNth(DblLinkedList *list, size_t index) {
	Node *elm = NULL;
	NodeItem *tmp = NULL;
	elm = getNthq(list, index);
	if (elm == NULL) {
		exit(5);
	}
	if (elm->prev) {
		elm->prev->next = elm->next;
	}
	if (elm->next) {
		elm->next->prev = elm->prev;
	}
	tmp = elm->value;

	if (!elm->prev) {
		list->head = elm->next;
	}
	if (!elm->next) {
		list->tail = elm->prev;
	}

	osFreeMem(elm);

	list->size--;

	return tmp;
}

/*
 * ������� �� ���������� ��������
 */
void insertBeforeElement(DblLinkedList *list, Node* elm, NodeItem *value) {
	Node *ins = NULL;
	if (elm == NULL) {
		exit(6);
	}

	if (!elm->prev) {
		pushFront(list, value);
		return;
	}
	ins = (Node*) osAllocMem(sizeof(Node));
	ins->value = value;
	ins->prev = elm->prev;
	elm->prev->next = ins;
	ins->next = elm;
	elm->prev = ins;

	list->size++;
}
//��������� ���� ��������� ������
int nodeitem_cmp(NodeItem* ni1, NodeItem* ni2) {

	if (strcmp((ni1->title), (ni2->title)) > 0)
		return 1;
	else
		return 0;

}
/*
 * ���������� ������ ���������
 */
void insertionSort(DblLinkedList **list, int (*cmp)(NodeItem*, NodeItem*)) {
	DblLinkedList *out = createDblLinkedList((*list)->editable);
	Node *sorted = NULL;
	Node *unsorted = NULL;

	pushFront(out, popFront(*list));

	unsorted = (*list)->head;
	while (unsorted) {
		sorted = out->head;
		while (sorted && cmp(unsorted->value, sorted->value)) {
			sorted = sorted->next;
		}
		if (sorted) {
			insertBeforeElement(out, sorted, unsorted->value);
		} else {
			pushBack(out, unsorted->value);
		}
		unsorted = unsorted->next;
	}

	deleteDblLinkedList(list);
	*list = out;
}


/*
 * ���������  �������� ������ ��������� ������ �������� ������
 * nodeitem - ��������� �� ������� ������
 * curnum - ���������� ����� ���������
 * ������������ ��������: 1 - �������, 0 - ���������
 * */
uint8_t editparamitem(NodeItem *nodeitem, int curnum) {

	param_table_item *paramnodeitem = (param_table_item *) (nodeitem->value
			+ curnum);

	//�������� ���������
	if (paramnodeitem->paramtype == paramtype_char) {

		free(paramnodeitem->paramitem);
		paramnodeitem->paramitem = strdup(paramitemstr);
		//���� ���������� �������� ������
		if ((curnum == 0) && ((nodeitem->type) ==good))
		{
			if (strlen(paramitemstr)>0)
			{
				cutstr(paramitemstr);
				free(nodeitem->title);
				nodeitem->title = strdup(paramitemstr);
			}
		}
		return 1;
	}
	else if ((paramnodeitem->paramtype == paramtype_int1) || (paramnodeitem->paramtype == paramtype_int4))
	{
		uint64_t s = 0;
		int i = 0;
		for (i = 0; paramitemstr[i] != '\0'; i++)
		{
			s = s * 10 + paramitemstr[i] - '0';
		}
		uint64_t maxval = 0;
		for (i = 1; i <= paramnodeitem->numplaces; i++)
		{
			maxval = maxval * 10 + 9;
		}
		if ((s > maxval)) // ���� �������� ������ ����������� ����������� ��� �������� ����� ������
		{
			key_pressed key;

			glcd_print("������ �����", 1);
			//�������� ������� ������ ����
			for (;;)
			{
				key.value = osMessageGet(kbdQueueHandle, osWaitForever).value.v;
				switch (key.key)
				{
				case KEY_CLEAR: //������ C
					return 0;
					break;
				}
			}

		}
		else if (((curnum == 3) && ((nodeitem->type) == good) && (s > list_departs->size)))
		{

			key_pressed key;

			glcd_print("������ �����", 1);
			//�������� ������� ������ ����
			for (;;)
			{
				key.value = osMessageGet(kbdQueueHandle, osWaitForever).value.v;
				switch (key.key)
				{
				case KEY_CLEAR: //������ C
					return 0;
					break;
				}
			}
		}
		switch(paramnodeitem->paramtype)
		{
		case paramtype_int1:
			*((uint8_t *)paramnodeitem->paramitem) = s;
			break;
		case paramtype_int4:
			*((uint32_t *)paramnodeitem->paramitem) = s;
			break;
		}
		return 1;

	}
	else if ((paramnodeitem->paramtype == paramtype_int8))
		{
		//��� paramtype_int8 ���������������!!!
		}
	else if ((paramnodeitem->paramtype == paramtype_float)) {

		uint64_t s_int = 0;
		double s_doub = 0;
		int i = 0;
		int k = 100;
		int k_coef = 1;
		for (i = 0; paramitemstr[i] != '\0'; i++) {
			if (paramitemstr[i] != '.') {
				s_int = s_int * 10 + paramitemstr[i] - '0';
			} else {
				k = i;
			}
			if (k < i) {
				k_coef = k_coef * 10;
			}
		}
		s_doub = ((double) s_int) / k_coef;
		uint64_t maxval=0;
				for (i = 1; i<=paramnodeitem->numplaces; i++) {
					maxval = maxval * 10 + 9;
						}
		if ((s_doub >= maxval+1)) // ���� �������� ������ ����������� �����������
		{
			key_pressed key;
			glcd_print("������ �����", 1);
			//�������� ������� ������ ����
			for (;;) {
				key.value = osMessageGet(kbdQueueHandle, osWaitForever).value.v;
				switch (key.key) {
				case KEY_CLEAR: //������ C
					return 0;
					break;
				}
			}
		}

		memcpy((double *) (paramnodeitem->paramitem), &s_doub, sizeof(double));
		return 1;
	}
	else if (paramnodeitem->paramtype == paramtype_char_tel)
	{
		int i = 0;
	int flag=0;
	if (strlen(paramitemstr)!=0)
	{
	if (paramitemstr[0] != '+')
		flag=1;
	else {
		for (i = 1; paramitemstr[i] != '\0'; i++) {
			if ((paramitemstr[i] <'0') || (paramitemstr[i] > '9')) {
				flag=1;
				break;
			}
		}
	}
	}
		//������ �����
	if (flag)
	{key_pressed key;
	//lcd_cl_row(1);
	glcd_print("������ �����", 1);
	//�������� ������� ������ ����
	for (;;) {
		key.value = osMessageGet(kbdQueueHandle, osWaitForever).value.v;
		switch (key.key) {
		case KEY_CLEAR: //������ C
			return 0;
			break;
		}
	}

	}

		free(paramnodeitem->paramitem);
			paramnodeitem->paramitem = strdup(paramitemstr);
			return 1;
	}
	return 1;
}

/*
 * ���������  �������� ������ ��������� ���� ticks ������ �������� ������
 * ���
 * ���������  �������� ������ ��������� ���� list
 * nodeitem - ��������� �� ������� ������
 * curnum - ���������� ����� ���������
 * */
void editparamitem_ticks_list(NodeItem *nodeitem, int curnum, int numtick) {

	param_table_item *paramnodeitem = (param_table_item *) (nodeitem->value
			+ curnum);

	//�������� ���������
	if ((paramnodeitem->paramtype == paramtype_ticks)) {
		((param_tick_item *) (((param_tick_item *) paramnodeitem->paramitem)
				+ numtick))->value = newvaltick;
		//paramnodeitem->paramitem=(void *)((char *)newvaltick);
	}
	if ((paramnodeitem->paramtype == paramtype_list)) {
		for (int i = 0; i < paramnodeitem->numplaces; i++) {
			if (i == numtick) {
				((param_tick_item *) (((param_tick_item *) paramnodeitem->paramitem)
						+ i))->value = 1;
			} else {
				((param_tick_item *) (((param_tick_item *) paramnodeitem->paramitem)
						+ i))->value = 0;
			}
		}

	}
}


/*
 * ������������ ��������:
 * NO_CHANGE - �� ���� ���������
 * CHANGE_ALL - ��������� ��������� (������� ���� ��� ������, ���� ������� �� � ������ ������� ��������� �����)
 * GOTO_R - ������� � ���� (��� ���������� ���������)
 */
uint8_t drawNode(NodeItem *nodeitem, int curnum, int numpar, int startpar) {
	uint8_t res = NO_CHANGE;

	bool ChangeFlag = false; //���� ��� �� ������� ���� �� 1 ��������
	while (1) {
		uint8_t FirstIter=5;
		char str_temp[MAXSTRLEN];
		//��������� ��� �������� � ������� ����� ������
		char* str_header = NULL;
		char* str_buf = NULL;
		uint8_t numplaces = 0;
		uint8_t oldvalue = 0;
		uint8_t *newvalue = NULL;

		param_table_item *paramnodeitem = (param_table_item *) (nodeitem->value
				+ curnum);
		// ������� ������
		glcd_cls();
		char buf[21];
		memset(buf, ' ', 21);
		//��������� (�������� ���������)
		char *title = (char *) (paramnodeitem->paramtitle);
		memcpy(buf+ ((MAXCOLS - 3 - strlen(title)) >> 1 ),(uint8_t *) title, strlen(title));
		if (numpar > (1 + (curnum - startpar)))
			memset(buf+MAXCOLS-1, ARROW_RIGHT,1);
		char buf_num[3];
			sprintf(buf_num, "%d", (curnum - startpar) + 1);
			memcpy(buf+ MAXCOLS-1 - strlen(buf_num) ,buf_num, strlen(buf_num));
			if ((curnum - startpar) > 0)
				memset(buf+MAXCOLS-2 - strlen(buf_num), ARROW_LEFT,1);
			buf[MAXCOLS]=0;
			glcd_print_at(buf, 0, 0);
		/*glcd_print_center((char *) (paramnodeitem->paramtitle), 0);


		  //������� � ����������
		if (numpar > (1 + (curnum - startpar)))
			glcd_put_at(ARROW_RIGHT, 0, 19);
		char buf[4];
		sprintf(buf, "%d", (curnum - startpar) + 1);
		glcd_print_at(buf, 0, 19 - strlen(buf));
		//lcd_putat((char) (((int) '0' + 1) + curnum) , 0,14);
		if ((curnum - startpar) > 0)
			glcd_put_at(ARROW_LEFT, 0, 18 - strlen(buf));
		 */
		int numval = 0;

		//����� ���� ���������
		//���������
		if (( paramnodeitem->paramtype == paramtype_char ) || (paramnodeitem->paramtype == paramtype_char_tel)) {

			clearmas();
			strcpy(paramitemstr, paramnodeitem->paramitem);
			str_header = (char *) (paramnodeitem->paramtitle);
			str_buf = paramitemstr;
			numplaces = (uint8_t) (paramnodeitem->numplaces);
			oldvalue = 0;
			newvalue = NULL;
		}
		//����� �����
		else if ((paramnodeitem->paramtype == paramtype_int1) || (paramnodeitem->paramtype == paramtype_int4) ||
				(paramnodeitem->paramtype == paramtype_int8))
		{
			char numberstring[14];
			switch(paramnodeitem->paramtype)
			{
			case paramtype_int1:
				sprintf(numberstring, "%hhu", *((uint8_t*) paramnodeitem->paramitem));
				break;
			case paramtype_int4:
				sprintf(numberstring, "%u", *((uint32_t*) paramnodeitem->paramitem));
				break;
			case paramtype_int8:
				//sprintf(numberstring, "%llu", *((uint64_t*) paramnodeitem->paramitem));
				sprintf(numberstring, "%llu.%02llu",*((uint64_t*) paramnodeitem->paramitem) / 100,
						*((uint64_t*) paramnodeitem->paramitem) % 100);//
				break;
			}

			glcd_print_at(numberstring, 1, 1);
			clearmas();
			strcpy(paramitemstr, numberstring);
			str_header = NULL;
			str_buf = paramitemstr;
			numplaces = (uint8_t) (paramnodeitem->numplaces);

			oldvalue = 0;
			newvalue = NULL;
		}
		//�������������� �����
		else if ((paramnodeitem->paramtype == paramtype_float)) {
			sprintf(paramitemstr, "%.2lf",
					*((double *) paramnodeitem->paramitem));
			glcd_print_at(paramitemstr, 1, 1);
			str_header = NULL;
			str_buf = paramitemstr;
			numplaces = 0;
			oldvalue = 0;
			newvalue = NULL;

		}
		//������ � ������� tick
		else if ((paramnodeitem->paramtype == paramtype_ticks))
		{

			numval = 0; // number of tick
			str_header = NULL;
			numplaces = 0;
		}
		//������ �������� list
		else if ((paramnodeitem->paramtype == paramtype_list)) {
			numval = 0;
			for (int i = 0; i < paramnodeitem->numplaces; i++) {
				if (((param_tick_item *) ((param_tick_item *) paramnodeitem->paramitem
						+ i))->value) {
					numval = i;
					break;
				}
			}
			str_header = NULL;
			numplaces = 0;
			oldvalue = 0;
		}

		for (;;) {
			if (FirstIter==5)
						FirstIter=1;
					else FirstIter=0;

			lcd_hidecursor();
			param_tick_item tick_item;

			switch (paramnodeitem->paramtype) {
			case paramtype_ticks:
				str_buf =
						(((param_tick_item *) (((param_tick_item *) paramnodeitem->paramitem)
								+ numval))->title);
				oldvalue =
						(((param_tick_item *) (((param_tick_item *) paramnodeitem->paramitem)
								+ numval))->value);
				newvaltick = 0;
				newvalue = &newvaltick;
				break;
			case paramtype_list:
				str_buf =
						((param_tick_item *) ((param_tick_item *) paramnodeitem->paramitem
								+ numval))->title;
				if (FirstIter){
					//��������� �������� ���������
					strcpy(str_temp, str_buf);
					oldvalue=0;
				}
				else {
					//��������� �� ������� �������� ��������� � ���������
					if (strcmp(str_buf, str_temp)==0)
					{
						oldvalue=0;
					}
					else oldvalue=1;
				}

				newvaltick = 0;
				newvalue = &newvaltick;
				break;
			}

			dep_input=false;

			if (paramnodeitem->editable) {
				res = inputFuncProto[paramnodeitem->paramtype](str_header,
						str_buf, numplaces, oldvalue, newvalue);
			} else {
				//����� ��������������� ��������
				res = input_noedit_FuncProto[paramnodeitem->paramtype](str_buf,
						oldvalue);
			}

			// ����� �� ������ ��������������
			if (res == CHANGE) { //������ ������� �� (������� �������� ����������������, �� ������� �� �������)
				res = NO_CHANGE;
				if (paramnodeitem->editable) {
					if ((paramnodeitem->paramtype == paramtype_int1) || (paramnodeitem->paramtype == paramtype_int4) ||
							(paramnodeitem->paramtype == paramtype_int8) || (paramnodeitem->paramtype == paramtype_float))
					{
						if (editparamitem(nodeitem, curnum)) {
							ChangeFlag = true;
							break;
						}

					} else {
						if ((paramnodeitem->paramtype == paramtype_char)||(paramnodeitem->paramtype == paramtype_char_tel)) {

							editparamitem(nodeitem, curnum);
						}else
						{
							editparamitem_ticks_list(nodeitem, curnum, numval);
						}
						ChangeFlag = true;
						break;
					}

				}
			} else if (res == CHANGE_ALL) { //������ ������� ���� (������� �������� �� ����������������, ����� �� �������)
				res = NO_CHANGE;
				lcd_hidecursor();
						if IsOptionsMenu(nodeitem->type)
						{
							if (SP > 0)
							{
								curmenu = menustack[--SP];
								temp= menu_pos_stack[SP];
								//drawmenu(curmenu, temp);
								selectmenu(curmenu, menu_setpos,temp);
							}
						}
						return CHANGE_ALL;
			} else if (res == NO_CHANGE) { //������ ������� �� (������� �������� �� ����������������, ����� �� �������)
				lcd_hidecursor();
				if IsOptionsMenu(nodeitem->type)
				{
					if (SP > 0)
					{
						curmenu = menustack[--SP];
						temp= menu_pos_stack[SP];
						//drawmenu(curmenu, temp);
						selectmenu(curmenu, menu_setpos,temp);
					}
				}
				if (ChangeFlag) //���� ����� ���� ���������, �� ���������� CHANGE_ALL
					return CHANGE_ALL;
				else
				return NO_CHANGE;
			}
			else
			if (res == GOTO_R) { //������� � ����
				lcd_hidecursor();
				if (SP > 0) {
					curmenu = (menuitem*) &topmenu;
					SP = 0;
					temp = 0;
					//drawmenu(curmenu, temp);
					selectmenu(curmenu, menu_setpos, temp);
				}
				return GOTO_R;
			} else {
				if (res == PREV) {
					if ((curnum - startpar) - 1 >= 0) {
						res = NO_CHANGE;
						curnum--;
						break;
					}
				}
				if (res == NEXT) {
					if ((curnum - startpar) + 1 < numpar) {
						res = NO_CHANGE;
						curnum++;
						break;
					}
				}
				if (res == PREV_TICK) {
					if (numval - 1 >= 0) {
						res = NO_CHANGE;
						numval--;
					}
				}
				if (res == NEXT_TICK) {
					if (numval + 1 < paramnodeitem->numplaces) {
						res = NO_CHANGE;
						numval++;
					}
				}
			}
		} // end of for
	} // end of while
}




/*
 * ����� ���� ���������� ������ �������� ������
 * list - ��������� �� ������
 * num - ����� �������� � ������
 * numpar - ���������� ���������� ��� ��������� ������� ������
 * startpar - �����  ���������� ��������� (�� ��������� - 0)
 * ������������ ��������: NO_CHANGE, CHANGE_ALL, GOTO_R
 * */
uint8_t ShowNode(DblLinkedList *list, int num, int numpar,int startpar) {
	Node* node = getNthq(list, (int) num);
	NodeItem *nodeitem = (NodeItem *) node->value;
	int curnum = startpar;
	uint8_t res= drawNode(nodeitem, curnum, numpar, startpar);
	return res;
}


/*
 * ������ + ����� ������ �������� ������� ������� (��� ��������������)
 * index - ����� ������ � �������
 * numdep - ����� ������ (� �������)
 */
uint8_t ShowNodeGood(uint16_t index, uint8_t numdep) {
	struct memory_good mygood;

	uint64_t p64=0;
	struct memory_good_index myind;
		GetIndex(IndexNames, index, numdep-1,&myind);
		 GetGood(myind.index, numdep-1, &mygood);

	//GetGoodByNum(index, numdep-1, &mygood);
	param_tick_item good_drob[2] = { { "�������", mygood.drob }, { "������", mygood.service } };

	//�������� ��������
	param_table_item *value = (param_table_item*) osAllocMem(
			sizeof(param_table_item) * NUM_PARAMS_GOOD);
	param_table_item params[] =
	{
			{ paramtype_char, MEMORY_GOOD_NAMESIZE, 0, "��������", NULL },
			{ paramtype_int4, 9, 0, "�����", (void *) (osAllocMem(sizeof(uint32_t))) }, //uint32_t
			{ paramtype_char, 13, 0, "��������", NULL}, //uint64_t
			{ paramtype_int1, 2, 0, "������",(void *) (osAllocMem(sizeof(uint8_t))) }, //��������� ������� � 1
			{ paramtype_float, 7, 1, "����", (void *) (osAllocMem(sizeof(double))) }, //����� �������������
			{ paramtype_ticks, 2, 0, "���������", (void *) (osAllocMem(sizeof(good_drob))) },

			 };

	params[0].paramitem = strdup(mygood.name);
	memcpy((uint32_t *) (params[1].paramitem), &(mygood.idcode),
			sizeof(uint32_t));
	params[2].paramitem = strdup(mygood.barcode);

	memcpy((uint8_t *) (params[3].paramitem), &(mygood.secnum),
			sizeof(uint8_t));

	double price = (double) mygood.price / 100;
	memcpy((double *) (params[4].paramitem), &(price), sizeof(double));

	memcpy((uint8_t *) (params[5].paramitem), good_drob, sizeof(good_drob));

	memcpy(value, params, sizeof(params));
	NodeItem* nodeitem = createNodeItem(good, "Title", value);
	uint8_t res= drawNode(nodeitem, 0, NUM_PARAMS_GOOD, 0);

	switch (res) {
		case NO_CHANGE:
		case GOTO_R:
			break;
		case CHANGE_ALL:
			//strcpy(mygood.name, params[0].paramitem);
			memcpy(&(price),(double *) (params[4].paramitem),  sizeof(double));
			p64 = DoubleToInt(price * 100);
			//memcpy(&mygood.barcode, &barcode, sizeof(mygood.barcode));
			memcpy(&mygood.price, &p64, sizeof(mygood.price));
			//mygood.drob = good_drob[0].value;
			//strcpy(mygood.units,  params[6].paramitem);

			wait_start();  //������� "���������..."
			//�������� �� � ������
			//uint8_t EditGood(struct memory_good mygood, uint16_t index)
			if (!(EditGood(mygood, myind.index)))
			{
				wait_stop(); //������ "���������..."
				glcd_print("������ �����",0);
				while (WaitForTotal("���� ��� ������")!=READY);
				res=NO_CHANGE;
			}
			else
			{
				wait_stop(); //������ "���������..."
			}
			break;
		}



	osFreeMem(value);
	osFreeMem(nodeitem);
	return res;
}

/*
 * ������ + ����� ������ �������� ������� �������� (��� ��������������)
 * index - �����
 */
uint8_t ShowNodeKlient(uint16_t index) {
	struct memory_klient myklient;
		struct memory_klient_index myind;
		GetKlientIndex( index, &myind);
		GetKlient(myind.index, &myklient);

	//�������� ��������
	param_table_item *value = (param_table_item*) osAllocMem(sizeof(param_table_item) * NUM_PARAMS_KLIENT);
	uint64_t p64;

		param_table_item params[] =
		{
				{ paramtype_char, MEMORY_KLIENT_NAMESIZE, 1, "���", NULL }, //0
				//{ paramtype_int4, 9, 0, "���",  (void *) (osAllocMem(sizeof(uint32_t))) }, //1
				{ paramtype_char, MEMORY_KLIENT_ID_READER_SIZE, 0, "��",  NULL }, //1
				{ paramtype_int1, 2,1, "����� ������", (void *) (osAllocMem(sizeof(uint8_t)))}, //2 //��������� ������� � 1
				{ paramtype_float, 8, 1, "������, ���", (void *) (osAllocMem(sizeof(double)))},//3
				{ paramtype_float, 8, 1, "�������, ���", (void *) (osAllocMem(sizeof(double)))},//4
				{ paramtype_int4, 9, 1, "������, %", (void *) (osAllocMem(sizeof(uint32_t))) }, //5
				{ paramtype_int4, 9, 1, "�������, %", (void *) (osAllocMem(sizeof(uint32_t))) }, //6
				{ paramtype_float, 10, 1, "�����",  (void *) (osAllocMem(sizeof(double))) }, //7
		};

	params[0].paramitem = strdup(myklient.name);
	//memcpy((uint32_t *) (params[1].paramitem), &(myklient.code),
		//	sizeof(uint32_t));
	params[1].paramitem = strdup(myklient.id_reader);

	memcpy((uint8_t *) (params[2].paramitem), &(myklient.dics_num),
			sizeof(uint8_t));

	double val = (double) myklient.disc_val / 100;
	memcpy((double *) (params[3].paramitem), &(val), sizeof(double));

	val = (double) myklient.ext_val / 100;
	memcpy((double *) (params[4].paramitem), &(val), sizeof(double));

	memcpy((uint32_t *) (params[5].paramitem), &(myklient.disc_proc),
			sizeof(uint32_t));
	memcpy((uint32_t *) (params[6].paramitem), &(myklient.ext_proc),
				sizeof(uint32_t));

	double val_sum = (double) myklient.sum / 100;
	memcpy((double *) (params[7].paramitem), &(val_sum), sizeof(double));

	memcpy(value, params, sizeof(params));


	NodeItem* nodeitem = createNodeItem(klient, "Title", params);
	uint8_t res = drawNode(nodeitem, 0, NUM_PARAMS_KLIENT, 0);

	switch (res) {
		case NO_CHANGE:
		case GOTO_R:
			break;
		case CHANGE_ALL:
			//edit klient
		{
			memcpy(&(val),(double *) (params[3].paramitem),  sizeof(double));
			p64 = DoubleToInt(val * 100);
			memcpy(&myklient.disc_val, &p64, sizeof(myklient.disc_val));

			memcpy(&(val),(double *) (params[4].paramitem),  sizeof(double));
					p64 = DoubleToInt(val * 100);
					memcpy(&myklient.ext_val, &p64, sizeof(myklient.ext_val));

				myklient.dics_num = *(uint8_t *)(params[2].paramitem);

				myklient.disc_proc = *(uint32_t *)(params[5].paramitem);
				myklient.ext_proc = *(uint32_t *)(params[6].paramitem);

				memcpy(&(val_sum),(double *) (params[7].paramitem),  sizeof(double));
				p64 = DoubleToInt(val_sum * 100);
				memcpy(&myklient.sum, &p64, sizeof(myklient.sum));

			wait_start();  //������� "���������..."
			//�������� �� � ������
			if (!(EditKlient(myklient, index)))
			{
				wait_stop(); //������ "���������..."
				glcd_print("������ �����",0);
				while (WaitForTotal("���� ��� ������")!=READY);
				res=NO_CHANGE;
			}
			else
			{
				wait_stop(); //������ "���������..."
			}
		}
			break;
		}

	osFreeMem(value);
	osFreeMem(nodeitem);
	return res;
}

/*
 * �������� + ����� ������ �������� ������� ������� (��� ��������������)
 * numdep - ����� ������ (� �������)
 * newgoodpos - ����������� ����� ������ � ������� ��������� �������
 */
uint8_t ShowCreateNodeGood( uint8_t numdep, uint16_t *newgoodpos)
{
	//�������� ���������
	struct memory_good mygood;
	param_tick_item good_drob[2] = { { "�������", 1 }, { "������", 0 } };
	double price;
	uint64_t p64;
	param_table_item params[] =
	{
			{ paramtype_char, MEMORY_GOOD_NAMESIZE, 1, "��������", NULL },
			{ paramtype_int4, 9, 1, "�����", (void *) &mygood.idcode }, //uint32_t
			{ paramtype_char, 13, 1, "��������",  NULL }, //uint64_t
			{ paramtype_int1, 2, 0, "�����", (void *) &mygood.secnum }, //��������� ������� � 1
			{ paramtype_float, 8, 1, "����", (void *) &price },
			{ paramtype_ticks, 2, 1, "���������", (void *) (osAllocMem(sizeof(good_drob))) },
			};

	params[0].paramitem = strdup("�������� ������");
	mygood.idcode = 0;
	params[2].paramitem = strdup("");
	mygood.secnum = numdep;
	mygood.drob = 1;mygood.service = 0;
	price = 0;
	memcpy((uint8_t *) (params[5].paramitem), good_drob,
			sizeof(good_drob));

	NodeItem* nodeitem = createNodeItem(good, "Title", params);
	uint8_t res = drawNode(nodeitem, 0, NUM_PARAMS_GOOD, 0);

	switch (res) {
	case NO_CHANGE:
	case GOTO_R:
		break;
	case CHANGE_ALL:
		strcpy(mygood.name, params[0].paramitem);
		p64 = DoubleToInt(price * 100);
		strcpy(mygood.barcode,  params[2].paramitem);
		memcpy(&mygood.price, &p64, sizeof(mygood.price));
		mygood.drob = good_drob[0].value;
		mygood.service = good_drob[1].value;

		wait_start();  //������� "���������..."
		//�������� �� � ������
		if (!(InsertGood(mygood, newgoodpos)))
		{
			wait_stop(); //������ "���������..."
			glcd_cls();
			glcd_print("������ �����",0);
			while (WaitForTotal("���� ��� ������")!=READY);
			res=NO_CHANGE;
		}
		else
		{
			wait_stop(); //������ "���������..."
		}
		break;
	}
	osFreeMem(params[0].paramitem);
	osFreeMem(params[2].paramitem);
	osFreeMem(params[5].paramitem);
	osFreeMem(nodeitem->title);
	osFreeMem(nodeitem);
	return res;
}

/*
 * �������� + ����� ������ �������� ������� �������� (��� ��������������)
  * newpos - ����������� ����� �  ��������� �������
 */
uint8_t ShowCreateNodeKlient(uint16_t *newpos )
{
	//�������� ���������
	struct memory_klient myklient;
	memset(&myklient, 0, sizeof(struct memory_klient ));
	double val=0, val_ext, val_sum=0;
	uint64_t p64;

	param_table_item params[] =
	{
			{ paramtype_char, MEMORY_KLIENT_NAMESIZE, 1, "���", NULL }, //0
		//	{ paramtype_int4, 9, 1, "���", (void *) &myklient.code }, //1
			{ paramtype_char, MEMORY_KLIENT_ID_READER_SIZE, 1, "��",  NULL }, //1
			{ paramtype_int1, 2, 0, "����� ������", (void *) &myklient.dics_num }, //2
			{ paramtype_float, 8, 1, "������, ���", (void *) &val }, //3
			{ paramtype_float, 8, 1, "�������, ���", (void *) &val_ext }, //4
			{ paramtype_int4, 9, 1, "������, %", (void *) &myklient.disc_proc }, //5
			{ paramtype_int4, 9, 1, "�������, %", (void *) &myklient.ext_proc }, //6
			{ paramtype_float, 10, 1, "�����", (void *) &val_sum }, //7
	};

	params[0].paramitem = strdup("��� �������");
	params[1].paramitem = strdup("");


	NodeItem* nodeitem = createNodeItem(klient, "Title", params);
	uint8_t res = drawNode(nodeitem, 0, NUM_PARAMS_KLIENT, 0);

	switch (res) {
	case NO_CHANGE:
	case GOTO_R:
		break;
	case CHANGE_ALL:
		strcpy(myklient.name, params[0].paramitem);
		strcpy(myklient.id_reader,  params[1].paramitem);
		p64 = DoubleToInt(val * 100);
		memcpy(&myklient.disc_val, &p64, sizeof(myklient.disc_val));
		p64 = DoubleToInt(val_ext * 100);
			memcpy(&myklient.ext_val, &p64, sizeof(myklient.ext_val));

		myklient.dics_num = *(uint8_t *)(params[2].paramitem);
		myklient.disc_proc = *(uint32_t *)(params[5].paramitem);
		myklient.ext_proc = *(uint32_t *)(params[6].paramitem);
		p64 = DoubleToInt(val_sum * 100);
		memcpy(&myklient.sum, &p64, sizeof(myklient.sum));

		wait_start();  //������� "���������..."
		//�������� �� � ������
		if (!(InsertKlient(myklient, newpos)))
		{
			wait_stop(); //������ "���������..."
			glcd_cls();
			glcd_print("������ �����",0);
			while (WaitForTotal("���� ��� ������")!=READY);
			res=NO_CHANGE;
		}
		else
		{
			wait_stop(); //������ "���������..."
		}
		break;
	}
	osFreeMem(params[0].paramitem);
	osFreeMem(params[1].paramitem);
	osFreeMem(nodeitem->title);
	osFreeMem(nodeitem);
	return res;
}

/*
��������� ����������  ���� �� ����
num - ���������� ����� ����
 * ������������ ��������:
 * NO_CHANGE
 * CHANGE_ALL
 * GOTO_R
 */
uint8_t ShowNodeRecFromControlLine(DblList *MyR,	uint32_t num) {
uint32_t k = MyR->size;
char string[17];
	//�������� ��������
	param_table_item *value = (param_table_item*) osAllocMem(
			sizeof(param_table_item) * (NUM_PARAMS_REC+2*k));
	param_table_item params[NUM_PARAMS_REC+2*k];

	params[0] = (param_table_item) {	.paramtype = paramtype_int4,
			.numplaces = MAX_DOCNUM_SIZE,
			.editable = 0,
			.paramtitle = "����� ����",
			.paramitem= (void *) (osAllocMem(sizeof(uint32_t)))
	};
	memcpy((uint32_t *) (params[0].paramitem), &(num),
				sizeof(uint32_t));

	params[1] = (param_table_item){ .paramtype = paramtype_char, .numplaces =15,
			.editable =0,
			.paramtitle = "����/�����",
			.paramitem= NULL};
	char string_date[20]; dt date;
					UnixToTM(&date, MyR->date);
					sprintf(string_date, "%4ld-%02d-%02dT%02d:%02d:%02d", date.tm_year,
							date.tm_mon, date.tm_mday, date.tm_hour, date.tm_min,
							date.tm_sec);
	params[1].paramitem = strdup(string_date);

	RPos *tmp = NULL;
	tmp = MyR->head;
	for (int i=0; i<2*k; i=i+2)
	{
	sprintf(string, "����� ������� %ld", i+1);

		params[2+i] =(param_table_item) { .paramtype =paramtype_int8,
				.numplaces = MAX_CASHREGISTER_SIZE,
				.editable =0,
				.paramtitle = NULL,
				.paramitem= (void *) (osAllocMem(sizeof(uint64_t)))};
		params[2+i].paramtitle = strdup(string);
		memcpy((uint64_t *) (params[2+i].paramitem), &(tmp->sum),
					sizeof(uint64_t));

		params[2+i+1] = (param_table_item){ .paramtype =paramtype_int1,
				.numplaces = MAX_CASHREGISTER_SIZE,
				.editable =0,
				.paramtitle = "������",
				.paramitem= (void *) (osAllocMem(sizeof(uint8_t)))};
			memcpy((uint8_t *) (params[2+i+1].paramitem), &(tmp->secnum),
						sizeof(uint8_t));
		//���������� �������� ������ - �������
		tmp = tmp->next;
	}
	params[2+2*k] =(param_table_item) { .paramtype =paramtype_int8,
			.numplaces = MAX_CASHREGISTER_SIZE,
			.editable =0,
			.paramtitle = "����� �����",
			.paramitem= (void *) (osAllocMem(sizeof(uint64_t)))};
	memcpy((uint64_t *) (params[2+2*k].paramitem), &(MyR->TOTAL),
						sizeof(uint64_t));

	params[2+2*k+1] = (param_table_item){ .paramtype =paramtype_int1,
			.numplaces = MAX_CASHREGISTER_SIZE,
			.editable =0,
			.paramtitle = "������",
			.paramitem= (void *) (osAllocMem(sizeof(uint8_t)))};
	memcpy((uint8_t *) (params[2+2*k+1].paramitem), &(MyR->KassirId),
				sizeof(uint8_t));
	params[2+2*k+2] = (param_table_item){ .paramtype =paramtype_int8,
			.numplaces = MAX_CASHREGISTER_SIZE,
			.editable =0,
			.paramtitle = "���������",
			.paramitem= (void *) (osAllocMem(sizeof(uint64_t)))};
	memcpy((uint64_t *) (params[2+2*k+2].paramitem), &(MyR->ForPayCash),
						sizeof(uint64_t));

	memset(string, 0, 17);
	strncpy(string, get_cash_type_name(ECash), 16);

	params[2+2*k+3] = (param_table_item){.paramtype =paramtype_int8,
			.numplaces = MAX_CASHREGISTER_SIZE,
			.editable =0,
			.paramtitle = NULL,
			.paramitem= (void *) (osAllocMem(sizeof(uint64_t)))};
	memcpy((uint64_t *) (params[2+2*k+3].paramitem), &(MyR->TOTAL_cash_type[ECash]),
							sizeof(uint64_t));
	params[2+2*k+3].paramtitle = strdup(string);

	memset(string, 0, 17);
	strncpy(string, get_cash_type_name(CashType2), 16);
	params[2+2*k+4] = (param_table_item) { .paramtype =paramtype_int8,
			.numplaces = MAX_CASHREGISTER_SIZE,
			.editable =0,
			.paramtitle = NULL,
			.paramitem= (void *) (osAllocMem(sizeof(uint64_t)))
	};
	params[2+2*k+4].paramtitle = strdup(string);
	memcpy((uint64_t *) (params[2+2*k+4].paramitem),
			&(MyR->TOTAL_cash_type[CashType2]),
							sizeof(uint64_t));
	memset(string, 0, 17);
		strncpy(string, get_cash_type_name(CashType3), 16);
	params[2+2*k+5] = (param_table_item){ paramtype_int8,
			.numplaces = MAX_CASHREGISTER_SIZE,
			.editable =0,
			.paramtitle =NULL,
			.paramitem= (void *) (osAllocMem(sizeof(uint64_t)))};
	memcpy((uint64_t *) (params[2+2*k+5].paramitem), &(MyR->TOTAL_cash_type[CashType3]),
							sizeof(uint64_t));
	params[2+2*k+5].paramtitle = strdup(string);
	memset(string, 0, 17);
		strncpy(string, get_cash_type_name(CashType4), 16);
	params[2+2*k+6] =(param_table_item){ .paramtype =paramtype_int8,
			.numplaces = MAX_CASHREGISTER_SIZE,
			.editable =0,
			.paramtitle = NULL,
			.paramitem= (void *) (osAllocMem(sizeof(uint64_t)))};
	memcpy((uint64_t *) (params[2+2*k+6].paramitem), &(MyR->TOTAL_cash_type[CashType4]),
							sizeof(uint64_t));
	params[2+2*k+6].paramtitle = strdup(string);
	memcpy(value, params, sizeof(params));
	NodeItem* nodeitem = createNodeItem(receipt, "Title", value);
	uint8_t res= drawNode(nodeitem, 0, NUM_PARAMS_REC + 2*k, 0);

	/*switch (res) {
		case NO_CHANGE:
		case GOTO_R:
			break;
		case CHANGE_ALL:
			//strcpy(mygood.name, params[0].paramitem);
			memcpy(&(price),(double *) (params[4].paramitem),  sizeof(double));
			p64 = DoubleToInt(price * 100);
			//memcpy(&mygood.barcode, &barcode, sizeof(mygood.barcode));
			memcpy(&mygood.price, &p64, sizeof(mygood.price));
			//mygood.drob = good_drob[0].value;
			//strcpy(mygood.units,  params[6].paramitem);

			wait_start();  //������� "���������..."
			//�������� �� � ������
			//uint8_t EditGood(struct memory_good mygood, uint16_t index)
			if (!(EditGood(mygood, myind.index)))
			{
				wait_stop(); //������ "���������..."
				glcd_cls();
				lcd_printat("������ �����",0,0);
				while (WaitForTotal("���� ��� ������")!=READY);
				res=NO_CHANGE;
			}
			else
			{
				wait_stop(); //������ "���������..."
			}
			break;
		}
*/

osFreeMem(params[1].paramitem);
for (int i=0; i<2*k; i=i+2)
	{
osFreeMem(params[2+i].paramtitle);
	}
osFreeMem(params[2+2*k+3].paramtitle);
osFreeMem(params[2+2*k+4].paramtitle);
osFreeMem(params[2+2*k+5].paramtitle);
osFreeMem(params[2+2*k+6].paramtitle);
	osFreeMem(value);
	osFreeMem(nodeitem);
	return res;
}


/*
 * ����� ��������� ������ �������� ������
 * list - ��������� �� ������
 * title - ��������� ��� ������
 * curnum - ���������� ����� �������� � ������
 * */
void drawnodeitem(DblLinkedList *list, char *title, int curnum) {
	Node* node = getNthq(list, (int) curnum);
	// ������� ������
	glcd_cls();

	// ������ ������
		char buf[21];
		memset(buf, ' ', 21);

		memcpy(buf+ ((MAXCOLS - 3 - strlen(title)) >> 1 ),(uint8_t *) title, strlen(title)); //��������� ����
		//������� � ����������
		if (list->size>0){
			if (list->size > (1 + curnum))
			memset(buf+MAXCOLS-1, ARROW_RIGHT,1);
		char buf_num[3];
		sprintf(buf_num, "%d", curnum+1);
		memcpy(buf+ MAXCOLS-1 - strlen(buf_num) ,buf_num, strlen(buf_num));
		if (curnum > 0)
			memset(buf+MAXCOLS-2 - strlen(buf_num), ARROW_LEFT,1);

		if (list == list_departs) //�������� ������
			glcd_print_at((char *) (node->value->value->paramitem), 1, 1);
		else
			//����
			glcd_print_at((char *) (node->value->title), 1, 1);
		}
		else
			{
			glcd_print_at("��� ������", 1, 1);
			}
		buf[MAXCOLS]=0;
		glcd_print_at(buf, 0, 0); //����� ���������
		/*
	//���������
	glcd_print_center(title, 0);
	if (list->size>0){
	//������� � ����������
	if (list->size > (1 + curnum))
		glcd_put_at(ARROW_RIGHT, 0, 19);
	char buf[4];
	sprintf(buf,"%d",curnum+1);
	glcd_print_at(buf, 0,19-strlen(buf));
	//lcd_putat((char) (((int) '0' + 1) + curnum) , 0,14);

	if (curnum > 0)
		glcd_put_at(ARROW_LEFT, 0, 18-strlen(buf));

	}
	else
	{
		glcd_print_at("��� ������", 1, 1);

	}
	*/
}


/*
 * �����  ������ �������� ������ (������ � ������� �������)
 * (��� �����������)
 * numpos - ����� �������
 * list - ��������� �� ������
 * title - ��������� ��� ������
 * curnum - ���������� ����� �������� � ������
 * */
void drawnodeitem_pos(DblLinkedList *list, char *title, int curnum, uint8_t numpos) {
	Node* node = getNthq(list, (int) curnum);
	// ������� ������
	glcd_cls();
	//���������
	glcd_print(title, 0);
	//������� � ����������
	if (list->size > (1 + curnum))
		glcd_put_at(ARROW_RIGHT, 0, 19);
	char buf[4];
	sprintf(buf,"%d",curnum+1);
	glcd_print_at(buf, 0,19-strlen(buf));
	//lcd_putat((char) (((int) '0' + 1) + curnum) , 0,14);

	if (curnum > 0)
		glcd_put_at(ARROW_LEFT, 0, 18-strlen(buf));

	char strmas[4];
	sprintf(strmas, "%d>", numpos);
	glcd_print(strmas, 1);
	//����
	glcd_print_at((char *) (node->value->title), 1, strlen(strmas));
}

/*
 * ����� ������������ ������ ������
 * index - ����� ������ (�������� � ������� ��������� �������)
 * numdep - ����� ������ (� �������)
 */
void drawGood(char *str_header, uint16_t  index, uint16_t listsize, uint8_t numdep) {
		// ������� ������
	glcd_cls();
	//���������
	glcd_print(str_header, 0);

	if (listsize==0)
	{
		glcd_print(">��������", 1);
	}
	else {

	//������� � ����������
	if (listsize > (1 + index))
		glcd_put_at(ARROW_RIGHT, 0, 19);
	char buf[4];
	sprintf(buf,"%d",index+1);
	glcd_print_at(buf, 0,19-strlen(buf));
	//lcd_putat((char) (((int) '0' + 1) + curnum) , 0,14);

	if (index > 0)
		glcd_put_at(ARROW_LEFT, 0, 18-strlen(buf));

	//����
	struct memory_good mygood;
	GetGoodByNum(index,numdep-1, &mygood);
	glcd_print_at((char *) (mygood.name), 1, 1);
	}
}

/*
 * ����� ����� ������ �������
 * index - ���������� �����  (�������� � �������)
 */
void drawKlient(char *str_header, uint16_t  index, uint16_t listsize) {
		// ������� ������
	glcd_cls();
	//���������
	glcd_print(str_header, 0);

	if (listsize==0)
	{
		glcd_print(">��������", 1);
	}
	else {

	//������� � ����������
	if (listsize > (1 + index))
		glcd_put_at(ARROW_RIGHT, 0, 19);
	char buf[4];
	sprintf(buf,"%d",index+1);
	glcd_print_at(buf, 0,19-strlen(buf));
	//lcd_putat((char) (((int) '0' + 1) + curnum) , 0,14);

	if (index > 0)
		glcd_put_at(ARROW_LEFT, 0, 18-strlen(buf));

	//����
	struct memory_klient mykl;
	struct memory_klient_index myind;
	GetKlientIndex( index,&myind);

	GetKlient(myind.index,&mykl);
	glcd_print_at((char *) (mykl.name), 1, 1);
	}
}

/*
 * ������ ���� ���������� ������ �������� ������
 * list - ��������� �� ������
 * num - ����� �������� � ������
 * numpar - ���������� ���������� ��� ��������� ������� ������
 * */
void PrintNode(DblLinkedList *list, int num, int numpar) {
	char string[MAXSTRLENFORPRINT];
	char tempstring[MAXSTRLENFORPRINT/4];
	int numval = 0;
	bool flag=false;
	Node* node = getNthq(list, (int) num);
	NodeItem *nodeitem = (NodeItem *) node->value;
	sprintf(string, "%s", (char *) (nodeitem->title));
	tprinter_print_text(string, SMALL_FONT, TP_CENTER);
	param_table_item *paramnodeitem;
	for (int i = 0; i < numpar; i++) {
		paramnodeitem = (param_table_item *) (nodeitem->value + i);
		switch (paramnodeitem->paramtype) {
		case paramtype_char:
			sprintf(string, "%s: %s", (char *) (paramnodeitem->paramtitle),
					(char *) (paramnodeitem->paramitem));
			break;

		case paramtype_int1:
			sprintf(string, "%s: %hhu", (char *) (paramnodeitem->paramtitle), *((uint8_t*) paramnodeitem->paramitem));
			break;

		case paramtype_int4:
			sprintf(string, "%s: %u", (char *) (paramnodeitem->paramtitle), *((uint32_t*) paramnodeitem->paramitem));
			break;

		case paramtype_int8:
			//sprintf(string, "%s: %llu", (char *) (paramnodeitem->paramtitle), *((uint64_t*) paramnodeitem->paramitem));
			sprintf(string, "%s: %llu.%02llu",(char *) (paramnodeitem->paramtitle), *((uint64_t*) paramnodeitem->paramitem) / 100,
									*((uint64_t*) paramnodeitem->paramitem) % 100);
			break;

			break;

		case paramtype_float:
			sprintf(string, "%s: %.2lf", (char *) (paramnodeitem->paramtitle),
					*((double *) paramnodeitem->paramitem));
			break;
		case paramtype_list:
			numval = 0;
			for (int j = 0; j < paramnodeitem->numplaces; j++) {
				if (((param_tick_item *) ((param_tick_item *) paramnodeitem->paramitem
						+ j))->value) {
					numval = j;
					break;
				}
			}
			sprintf(string, "%s: %s", (char *) (paramnodeitem->paramtitle),
					(char *) (((param_tick_item *) ((param_tick_item *) paramnodeitem->paramitem
							+ numval))->title) );
			break;
		case paramtype_ticks:
			numval = 0;
			flag=false;
			memset(tempstring, 0, MAXSTRLENFORPRINT/4);
						for (int j = 0; j < paramnodeitem->numplaces; j++) {
							if (((param_tick_item *) ((param_tick_item *) paramnodeitem->paramitem
									+ j))->value) {

								numval = j;
								if (flag) {
									strcat(tempstring, ", ");
								}
								strcat(tempstring,(((param_tick_item *) (((param_tick_item *) paramnodeitem->paramitem)
										+ numval))->title));
								flag=true;
							}
						}
			sprintf(string, "%s: %s", (char *) (paramnodeitem->paramtitle),
											tempstring);
			break;
		default:
			string[0]=0;
			break;
		}
		tprinter_print_text(string, SMALL_FONT, TP_LEFT);
	}
	tprinter_print_text("\n", SMALL_FONT, TP_CENTER);
	tprinter_print_hline();
	tprinter_feed_out();
}

/*
 * ������ ���� ��������� ������
 * list - ��������� �� ������
 * numpar - ���������� ���������� ��� ��������� ������� ������
 * */
void PrintAllNodes(DblLinkedList *list,  int numpar) {
	char string[48];
 /*if	(list==(DblLinkedList*) list_goods)
 { sprintf(string, "�������\n%s", "������/������");

 }  else
	 if	(list==(DblLinkedList*) list_klients)
 {sprintf(string, "�������\n%s", "�������");

 }
 else */

	 if	(list==(DblLinkedList*) list_users)
 {sprintf(string, "�������\n%s", "������������");

 }
 else if	(list==(DblLinkedList*) list_departs)
 {sprintf(string, "�������\n%s", "������");

 }
 else if	(list==(DblLinkedList*) list_fixes)
 {sprintf(string, "�������\n%s", "��������� ������");

 }
 else if	(list==(DblLinkedList*) list_cashtypes)
 {sprintf(string, "�������\n%s", "������� ������");
 }
 else if (list==(DblLinkedList*) list_discounts)
 {sprintf(string, "�������\n%s", "������ � �������");

 } else if (list==(DblLinkedList*) list_advs)
 {sprintf(string, "�������\n%s",  "��������� ������");

 }

 	tprinter_print_text(string,LARGE_FONT, TP_CENTER);
	tprinter_print_text("\n",SMALL_FONT, TP_CENTER);
 	sprintf(string, "���������� �������: %d",  list->size);
 	tprinter_print_text(string,SMALL_FONT, TP_CENTER);
 		tprinter_print_text("\n",SMALL_FONT, TP_CENTER);

	for (int i=0; i<list->size; i++)
		{
		PrintNode(list, i,  numpar);
		}
}

/*
 * ������ ���� ���������� ������ ������
 * index - ����� ������ (�������� � ������� ��������� �������)
 * numdep - ����� ������ (� �������)
 * */
void PrintGood( uint16_t  index, uint8_t numdep) {
	struct memory_good mygood;
	GetGoodByNum(index,numdep-1, &mygood);
	char string[MAXSTRLENFORPRINT];
	sprintf(string, "��������: %s", mygood.name);
	tprinter_print_text(string, SMALL_FONT, TP_LEFT);
	sprintf(string, "���: %lu", mygood.idcode);
	tprinter_print_text(string, SMALL_FONT, TP_LEFT);
	sprintf(string, "�����-���: %s", mygood.barcode);
	tprinter_print_text(string, SMALL_FONT, TP_LEFT);
	sprintf(string, "������: %d", mygood.secnum);
	tprinter_print_text(string, SMALL_FONT, TP_LEFT);
	sprintf(string, "����: %lld.%02lld", mygood.price/100, mygood.price%100);
	tprinter_print_text(string, SMALL_FONT, TP_LEFT);
	sprintf(string, "�������: %d", mygood.drob);
	tprinter_print_text(string, SMALL_FONT, TP_LEFT);
	sprintf(string, "������: %d", mygood.service);
		tprinter_print_text(string, SMALL_FONT, TP_LEFT);
	tprinter_print_text("\n", SMALL_FONT, TP_CENTER);
	tprinter_print_hline();
	tprinter_feed_out();
}

/*
 * ������ ���� ���������� ������ �������
 * */
void PrintKlient( uint16_t  index) {
 	struct memory_klient myklient;
 	struct memory_klient_index myind;
 	GetKlientIndex( index,&myind);
 	GetKlient(myind.index, &myklient);

	char string[MAXSTRLENFORPRINT];
	sprintf(string, "��� �������: %s", myklient.name);
	tprinter_print_text(string, SMALL_FONT, TP_LEFT);
//	sprintf(string, "���: %lu", myklient.code);
//	tprinter_print_text(string, SMALL_FONT, TP_LEFT);
	sprintf(string, "��: %s", myklient.id_reader);
	tprinter_print_text(string, SMALL_FONT, TP_LEFT);
	sprintf(string, "������, %: %lu", myklient.disc_proc);
	tprinter_print_text(string, SMALL_FONT, TP_LEFT);
	sprintf(string, "�������, %: %lu", myklient.ext_proc);
		tprinter_print_text(string, SMALL_FONT, TP_LEFT);
	sprintf(string, "����� ������: %d", myklient.dics_num);
		tprinter_print_text(string, SMALL_FONT, TP_LEFT);
	sprintf(string, "������, ���.: %lld.%02lld", myklient.disc_val/100, myklient.disc_val%100);
	tprinter_print_text(string, SMALL_FONT, TP_LEFT);
	sprintf(string, "�������, ���.: %lld.%02lld", myklient.ext_val/100, myklient.ext_val%100);
		tprinter_print_text(string, SMALL_FONT, TP_LEFT);
	sprintf(string, "�����, ���.: %lld.%02lld", myklient.sum/100, myklient.sum%100);
		tprinter_print_text(string, SMALL_FONT, TP_LEFT);

	tprinter_print_text("\n", SMALL_FONT, TP_CENTER);
	tprinter_print_hline();
	tprinter_feed_out();
}

/*
 * ������ ���� �������
 * numdep - ����� ������ (� �������)
 * */
void PrintAllGoods(uint8_t numdep, uint16_t listsize) {
	char string[48];

	 sprintf(string, "�������\n%s", "������/������");
	 	tprinter_print_text(string,LARGE_FONT, TP_CENTER);
		tprinter_print_text("\n",SMALL_FONT, TP_CENTER);
	 	sprintf(string, "����� ������: %d",  numdep);
	 	tprinter_print_text(string,SMALL_FONT, TP_CENTER);
	 		tprinter_print_text("\n",SMALL_FONT, TP_CENTER);
	 	sprintf(string, "���������� �������: %d",  listsize);
	 	tprinter_print_text(string,SMALL_FONT, TP_CENTER);
	 		tprinter_print_text("\n",SMALL_FONT, TP_CENTER);

	for (int i=0; i<listsize; i++)
	{
		PrintGood( (uint16_t)  i,  numdep);
	}
}
/*
 * ������ ���� ��������
 * */
void PrintAllKlients(uint16_t listsize) {
	char string[48];

	 sprintf(string, "�������\n%s", "�������");
	 	tprinter_print_text(string,LARGE_FONT, TP_CENTER);
		tprinter_print_text("\n",SMALL_FONT, TP_CENTER);

	 	sprintf(string, "���������� ��������: %d",  listsize);
	 	tprinter_print_text(string,SMALL_FONT, TP_CENTER);
	 		tprinter_print_text("\n",SMALL_FONT, TP_CENTER);

	for (int i=0; i<listsize; i++)
	{
		PrintKlient( (uint16_t)  i);
	}
}

/*
 * �����  ������
 * list - ��������� �� ������
 * title - ��������� ��� ������
 * numpar - ���������� ���������� ��� ������� ��������
 * ������������ ��������: 1 - ���� ��������� �����-���� ��������� (���������/������� ��������, ��������������� ��������� ���������)
 * */
uint8_t ShowList(DblLinkedList *list, char *title, int numpar) {
	uint8_t Change=0; //������� �� ���������
	int curnum = 0; //���������� ����� �������� � ������
	uint8_t res = NO_ACTION;
	kbd_input_cur_mode = modeKIR_UP;
	drawnodeitem(list, title, curnum);
	key_pressed key;
	bool print_success = false;
	for (;;) {
		key.value = osMessageGet(kbdQueueHandle, osWaitForever).value.v;
		switch (key.key) {
		// ��������� (������ ��� ������������� �������)
			case KEY_0:
			case KEY_1:
			case KEY_2:
			case KEY_3:
			case KEY_4:
			case KEY_5:
			case KEY_6:
			case KEY_7:
			case KEY_8:
			case KEY_9:
			case KEY_00:
			case KEY_dot:
				if (list->editable) {
					res = str_searchInput(paramitemstr, key.key);//������ //�������

				if (res == NO_ACTION) {
					drawnodeitem(list, title, curnum);
					lcd_hidecursor();
				} else if (res == READY) {
					char c = paramitemstr[0];
					Node *node;
					char *s;
					bool fl = false;
					for (int i = 0; i < list->size; i++) {
						node = getNthq(list, i);
						s = (char *) ((NodeItem*) (node->value))->title;
						if (*(s) >= c) {
							fl = true;
							curnum = i;
							break;
						}
					}
					if (!(fl))
						curnum = list->size - 1;

					drawnodeitem(list, title, curnum);
					lcd_hidecursor();
				} else if (res == ANNULPOS)
					{
					drawnodeitem(list, title, curnum);
					lcd_hidecursor();
					}
				else if (res == ANNULR)
					{
					glcd_cls();
					lcd_hidecursor();
								SP = 0;
								curmenu = (menuitem*) &topmenu;
								MySession.state &= ~(show_clock);
								//����� �������� ����
							drawmenu(curmenu, 0);
							selectmenu(curmenu, menu_reset, 0);
									return Change;
					}
				}
				break;
		case KEY_MODE: //������ ������ ������
			glcd_cls();
			SP = 0;
			curmenu = (menuitem*) &topmenu;
			MySession.state &= ~(show_clock);
			//����� �������� ����
			drawmenu(curmenu, 0);
			selectmenu(curmenu, menu_reset, 0);
				return Change;
			break;
		case KEY_DISCOUNT:		//up
			if (curnum - 1 >= 0) {
				curnum--;
				drawnodeitem(list, title, curnum);
			}
			break;
		case KEY_EXTRA:		//down
			if (curnum + 1 < list->size) {
				curnum++;
				drawnodeitem(list, title, curnum);
			}
			else if (curnum + 1==list->size) {
				if (list->editable) {

					if (((list==list_departs)&&(list->size<SIZE_LIST_DEPARTS))
							||((list==list_advs)&&(list->size<SIZE_LIST_ADVS))

					)
					{//�������� ����� "��������"
					curnum++;
					//lcd_cl_row(1);
					glcd_print(">��������", 1);
					}
				}
			}
			break;
		case KEY_TOTAL: //OK
			if ((curnum==list->size)&&(list->editable)) {

							res=NO_ACTION;
								glcd_cls();
								glcd_print("�������� �������?", 1);
								//�������� ������� ������ ����
												for (;;) {
													key.value = osMessageGet(kbdQueueHandle, osWaitForever).value.v;
													switch (key.key) {
													case KEY_TOTAL: //������ ����
														res=READY;
														break;
													case KEY_ANNUL:
														res= ANNULPOS;
														break;
													}
													if (res!=NO_ACTION)
													{
														break;
													}
												}

								if (res==READY) {
								//������� ��������� �� ����������  �� ��������� //�������� ��������� � ������
								if ((list==list_departs)&&(list_departs->size<SIZE_LIST_DEPARTS))
									{
									char title[MEMORY_TITLESIZE+1];
									sprintf(title,  "������ N%d", curnum+1);
									struct memory_depart mydep;
										strcpy(mydep.name, title);
										strcpy(mydep.title, title);
										mydep.fixcode = 1;
										InitDepart(&mydep);
										Change=1;
									}
								else if ((list==list_advs)&&(list_advs->size<SIZE_LIST_ADVS))
								{
								char title[MEMORY_TITLESIZE+1];
								sprintf(title,  "�����%d", curnum+1);
								struct memory_advs myadv;
									strcpy(myadv.text, "");
									strcpy(myadv.title, title);
									myadv.place = 0;
									myadv.print_flag = 0;
									InitAdvs(&myadv);
									Change=1;
								}
								}
								else if (res==ANNULPOS)
								{
									drawnodeitem(list, title, curnum-1);
									//lcd_cl_row(1);
									glcd_print(">��������", 1);
									break;
								}
			}
			//������� � �������������� ����������/���������� �������� ������
			res = ShowNode(list, curnum, numpar,0);
			switch (res) {
			case NO_CHANGE:
				break;
			case CHANGE_ALL:
				Change=1;
				break;
			case GOTO_R:
				return Change;
				break;
			}
			drawnodeitem(list, title, curnum);
			break;

		case KEY_ANNUL: //EXIT, �����
			return Change;
			break;
		case KEY_C4: //������ �������� ��������
			if ((curnum<list->size)) {
				while (1) {
						wait_start();
						PrintNode(list, curnum, numpar); //����������� ������� ������� ������
							if (PrintFinal(1) == READY)
								break;
						}
					drawnodeitem(list, title, curnum);
			}
			break;


		case KEY_X: //������ ���� �������
			while (1) {
					wait_start();
					PrintAllNodes(list, numpar);
						if (PrintFinal(1) == READY)
							break;
					}
			 				if ((curnum<list->size)) {
			 					drawnodeitem(list, title, curnum);
			 				}
			 				else
			 				{drawnodeitem(list, title, curnum-1);
							//lcd_cl_row(1);
							glcd_print(">��������", 1);
			 				}
			break;

		case KEY_CLEAR: //�������� �������� ������
			if ((curnum!=list->size)&&(list->editable)) {
				res = NO_ACTION;
				glcd_cls();
				glcd_print("������� �������?", 1);
			//�������� ������� ������ ����
				for (;;) {
					key.value = osMessageGet(kbdQueueHandle, osWaitForever).value.v;
					switch (key.key) {
					case KEY_TOTAL: //������ ����
						res=READY;
						break;
					case KEY_ANNUL:
						res= ANNULPOS;
						break;
					}
					if (res!=NO_ACTION)
					{
						break;
					}
				}
				if (res==READY)
				{
					//�������� �������� ������
					deleteNth(list, curnum);
					Change=1;
					if (curnum>=list->size)
					{	curnum--;
					}
				}
				drawnodeitem(list, title, curnum);
			}
			break;
		}
	}
}


/*
 * ����� �������� LAN
 *
void ShowListConnectLAN() {
	uint8_t res=NO_CHANGE;
	key_pressed key;
	res=ShowNode(list_LANconnect, 0, NUM_PARAMS_CONNECT_LAN,0);
switch(res){
	case NO_CHANGE:
		break;
	case CHANGE_ALL:

		// �������� ������������ ����� ip-�������
		if (CheckLANSettings() == 0) {
			glcd_cls();
			lcd_printat("������ �����", 0, 0);
			lcd_printat("���� ��� ������", 1, 0);
			//�������� ������� ������ ����
			for (;;) {
				key.value = osMessageGet(kbdQueueHandle, osWaitForever).value.v;
				switch (key.key) {
				case KEY_TOTAL: //������ ����
					return;
					break;
				}
			}
		}
		else
		 //	������ ��������� � context
		UpdateConnectSettings(&context, connectLAN);
		break;
	case GOTO_R:
		break;
	}

}*/

/*
 * ����� ����� ��������  �����
 *
void ShowListConnectGeneral() {
	uint8_t res=NO_CHANGE;
	res=ShowNode(list_GeneralNetConnect, 0, NUM_PARAMS_CONNECT_GENERAL,0);
switch(res){
	case NO_CHANGE:
		break;
	case CHANGE_ALL:
		//	������ ��������� � context
		UpdateConnectSettings(&context, connectGeneral);
		break;
	case GOTO_R:
		break;
	}

}*/

/*
 * �����  ������ �������
 * */
void ShowListFixes() {

	uint8_t r = ShowList(list_fixes, "��������� ������", NUM_PARAMS_FIX);
	if (r)
			UpdateFixes();
}

/*
 * �����  ������ ������/�������
 * */
void ShowListDiscounts() {
	uint8_t r = ShowList(list_discounts, "������/�������", NUM_PARAMS_DISCOUNTS);
	if (r)
			UpdateDiscs();
}
/*
 * �����  ������ ��������� �����
 **/
void ShowListAdvs() {

	uint8_t r = ShowList(list_advs, "��������� ������", NUM_PARAMS_ADVS);
	if (r)
	{
		 UpdateAdvs();
	}
}


/*
 * �����  ������ �������� ������
 * */
void ShowListCashTypes() {
uint8_t res=	ShowList(list_cashtypes, "������� ������", NUM_PARAMS_CASH);
if (res)
	{
		UpdateCashTypes();
	}
}


void ShowListPrinters() {

uint8_t res=	ShowNode(list_printers, 0, 1,0);
switch(res){
	case NO_CHANGE:
		break;
	case CHANGE_ALL:
		//	������ ��������� � context
		UpdatePriters(&context);
		break;
	case GOTO_R:
		break;
	}
}

/*
 * �����  �������� ���
 * */
void ShowListKKMOptions() {
uint8_t res=	ShowNode(list_kkm_options, 0, NUM_PARAMS_KKM,0);
switch(res){
	case NO_CHANGE:
		break;
	case CHANGE_ALL:
		//	������ ��������� � context
		UpdateOptions(&context, kkm_opt) ;
		break;
	case GOTO_R:
		break;
	}
}

/*
 * �����  �������� ��������
 * */
void ShowListPrintOptions() {

	//ShowList(list_print_options, "������", NUM_PARAMS_PRINT);
	uint8_t res=ShowNode(list_print_options, 0, NUM_PARAMS_PRINT,0);
	switch(res){
		case NO_CHANGE:
			break;
		case CHANGE_ALL:
			//	������ ��������� � context
			UpdateOptions(&context, print_opt) ;
			break;
		case GOTO_R:
			break;
		}
}


/*
 * �����  �������� �����
 *
void ShowListWeightsOptions() {

//	ShowList(list_kkm_options, "���", NUM_PARAMS_KKM);
	uint8_t res=ShowNode(list_weights_options, 0, NUM_PARAMS_DEV_WEIGHTS,0);
	switch(res){
			case NO_CHANGE:
				break;
			case CHANGE_ALL:
				//	������ ��������� � context
				UpdateOptions(&context, weights_opt);
				HAL_UART_DeInit(&huart4);
				HAL_UART_DeInit(&huart5);
				if (context.options.KassaDevicesOptions.WeightsOptions.DevConnect)
				{
					//HAL_UART_Init(&huart4);
					MX_UART4_Init();
					weight_init();
				}
				if (context.options.KassaDevicesOptions.TerminalOptions.DevConnect)
				{
					MX_UART5_Init();
					//HAL_UART_Init(&huart5);
				}
				break;
			case GOTO_R:
				break;
			}
}*/

/*
 * �����  �������� ��������� �����
 * */
void ShowListMoneyBoxOptions() {

	uint8_t res=ShowNode(list_moneybox_options, 0, NUM_PARAMS_DEV,0);
	switch(res){
			case NO_CHANGE:
				break;
			case CHANGE_ALL:
				//	������ ��������� � context
				UpdateOptions(&context, moneybox_opt) ;
				break;
			case GOTO_R:
				break;
			}
}

/*
 * �����  �������� ����������
 * */
void ShowListRfidReaderOptions() {

	uint8_t res=ShowNode(list_rfidreader_options, 0, NUM_PARAMS_DEV,0);
	switch(res){
			case NO_CHANGE:
				break;
			case CHANGE_ALL:
				//	������ ��������� � context
				UpdateOptions(&context, rfidreader_opt) ;
				break;
			case GOTO_R:
				break;
			}
}

/*
 * �����  ��������  �������� ����������
 * */
void ShowListExtIndicatorOptions() {

	uint8_t res=ShowNode(list_ext_indicator_options, 0, NUM_PARAMS_DEV,0);
	switch(res){
			case NO_CHANGE:
				break;
			case CHANGE_ALL:
				//	������ ��������� � context
				UpdateOptions(&context, extindicator_opt) ;
				break;
			case GOTO_R:
				break;
			}
}

/*
 * �����  �������� ���������
 * */
void ShowListTerminalOptions() {

	uint8_t res=ShowNode(list_terminal_options, 0, NUM_PARAMS_DEV_TERM,0);
	switch(res){
			case NO_CHANGE:
				break;
			case CHANGE_ALL:
				//	������ ��������� � context
				UpdateOptions(&context, term_opt) ;
				break;
			case GOTO_R:
				break;
			}
}

/*
 * �����  �������� ����� (������-���������)
 * */
void ShowListOnlineSettings() {
uint8_t res = ShowNode(list_connect, 0, NUM_PARAMS_CONNECT,0);
switch(res){
	case NO_CHANGE:
		break;
	case CHANGE_ALL:
		//	������ ��������� � context
		UpdateConnectSettings(&context);
		break;
	case GOTO_R:
		break;
	}
}

/*
 * ����� �������� ����/�������
 * */
void ShowListDateTime(DblLinkedList *list) {
	uint8_t res;
	RTC_TimeTypeDef sTime;
	RTC_DateTypeDef sDate;
	res = ShowNode(list, 0, 5, 0);
	Node* node = getNthq(list, 0);
	NodeItem *nodeitem = (NodeItem *) node->value;
	param_table_item *paramnodeitem;

	switch (res) {
	case NO_CHANGE:
		break;

	case CHANGE_ALL:
		paramnodeitem = (param_table_item *) (nodeitem->value + 0);
		sDate.Year= (*(uint8_t *) paramnodeitem->paramitem);
		paramnodeitem = (param_table_item *) (nodeitem->value + 1);
		sDate.Month= (int) (*(uint8_t *) paramnodeitem->paramitem);

		if (sDate.Month > 12)
			sDate.Month = 12;

		paramnodeitem = (param_table_item *) (nodeitem->value + 2);
		sDate.Date = (int)(*(uint8_t *) paramnodeitem->paramitem);

		if (sDate.Date  > 31)
			sDate.Date  = 31;

		sDate.WeekDay = 1;

		paramnodeitem = (param_table_item *) (nodeitem->value + 3);
		sTime.Hours = *(uint8_t *) paramnodeitem->paramitem;
		if (sTime.Hours > 23)
			sTime.Hours = 0;
		paramnodeitem = (param_table_item *) (nodeitem->value + 4);
		sTime.Minutes = *(uint8_t *) paramnodeitem->paramitem;

		if (sTime.Minutes > 59)
			sTime.Minutes = 0;
		sTime.Seconds = 0;
		sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
		sTime.StoreOperation = RTC_STOREOPERATION_RESET;

		if (CheckDateTime(sDate, sTime) != -1) {

			if ((HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK) ||
					(HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK))
			{
				while (WaitForTotal("������ ���������") != READY)
					;
			}
		} else {
			while (WaitForTotal("������ �����") != READY)
				;

		}
		break;
	case GOTO_R:

		break;
	}

}

/*
 * �����  ������ �������������
 * */
void ShowListUsers() {

uint8_t r=	ShowList(list_users, "���������", NUM_PARAMS_USER);

if (r)
	UpdateUsers();
}
/*
 * �����  ������ �������
 * */
void ShowListDeparts() {
	uint8_t r = ShowList(list_departs, "������", NUM_PARAMS_DEPART);
	if (r)
		UpdateDeparts();
}


/*
 * �����  ������ ������� � ������ ����������������
 * */
void ShowListKlients()
{	uint8_t res = NO_ACTION;
	char numberstring[14];
	char * str_header = "�������";
	uint16_t goodscount = 0; //���������� ��������, � �� �������
	error_t er = memory_i_ReadKlientCount(&goodscount);
	int curnum = 0; //���������� ����� �������� � ������
	drawKlient(str_header, curnum, goodscount);
	lcd_hidecursor();
	key_pressed key;
	bool print_success = false;

	kbd_input_cur_mode = modeKIR_UP;

	for (;;)
	{
		key.value = osMessageGet(kbdQueueHandle, osWaitForever).value.v;
		switch (key.key)
		{
		case KEY_DISCOUNT:		//up
			if (curnum - 1 >= 0)
			{
				curnum--;
				drawKlient(str_header, curnum, goodscount);
				lcd_hidecursor();
			}
			break;
		case KEY_EXTRA:		//down
			if (curnum + 1 < goodscount)
			{
				curnum++;
				drawKlient(str_header, curnum, goodscount);
				lcd_hidecursor();
			}
			else if (curnum + 1 == goodscount)
			{
				//�������� ����� "��������"
				curnum++;
				//lcd_cl_row(1);
				glcd_print(">��������", 1);
			}

			break;
		case KEY_ANNUL: // ���������� �������������� ������ � ������� ���������
			return;
			break;
		case KEY_MODE:
			glcd_cls();
			SP = 0;
			curmenu = (menuitem*) &topmenu;
			MySession.state &= ~(show_clock);
			//����� �������� ����
			drawmenu(curmenu, 0);
			selectmenu(curmenu, menu_reset, 0);
			return;
			break;
		case KEY_TOTAL:
			if ((curnum == goodscount))
			{
				res = NO_ACTION;
				glcd_cls();
				glcd_print("�������� �������?", 1);
				//�������� ������� ������ ����
				for (;;)
				{
					key.value =
							osMessageGet(kbdQueueHandle, osWaitForever).value.v;
					switch (key.key)
					{
					case KEY_TOTAL: //������ ����
						res = READY;
						break;
					case KEY_ANNUL:
						res = ANNULPOS;
						break;
					}
					if (res != NO_ACTION)
					{
						break;
					}
				}

				if (res == READY)
				{
uint16_t newpos;
					//������� ��������� �� ����������  �� ��������� //�������� ��������� � ������
					res = ShowCreateNodeKlient(&newpos);
					switch (res)
					{
					case NO_CHANGE:
						drawKlient(str_header, curnum - 1, goodscount);
						glcd_print(">��������", 1);
						break;
					case CHANGE_ALL:
					{
						error_t er = memory_i_ReadKlientCount(&goodscount);
						curnum = (int) newpos;
						drawKlient(str_header, curnum, goodscount);
					}
						break;
					case GOTO_R:
						glcd_cls();
						SP = 0;
						curmenu = (menuitem*) &topmenu;
						MySession.state &= ~(show_clock);
						//����� �������� ����
						drawmenu(curmenu, 0);
						selectmenu(curmenu, menu_reset, 0);
						return;
						break;
					}

				}
				else if (res == ANNULPOS)
				{
					drawKlient(str_header, curnum - 1, goodscount);

					glcd_print(">��������", 1);
					break;
				}
			}
			else
			{
				//������� � �������������� ����������/���������� �������� ������
				res = ShowNodeKlient(curnum);

				switch (res)
				{
				case NO_CHANGE:
					break;
				case CHANGE_ALL:
					break;
				case GOTO_R:
					glcd_cls();
					SP = 0;
					curmenu = (menuitem*) &topmenu;
					MySession.state &= ~(show_clock);
					//����� �������� ����
					drawmenu(curmenu, 0);
					selectmenu(curmenu, menu_reset, 0);
					return;
					break;
				}
				drawKlient(str_header, curnum, goodscount);
			}
			break;

			// ���� ������ ������
		case KEY_0:
		case KEY_1:
		case KEY_2:
		case KEY_3:
		case KEY_4:
		case KEY_5:
		case KEY_6:
		case KEY_7:
		case KEY_8:
		case KEY_9:
		case KEY_00:
		case KEY_dot:
		 //����� ��������
			 res = str_searchInput(paramitemstr, key.key);
			 if (res == NO_ACTION) {
			 drawKlient(str_header, curnum, goodscount);
			 lcd_hidecursor();
			 } else if (res == READY) {
			 char c = paramitemstr[0];

			 curnum = binarysearch_klient_first_symbol(c, goodscount);
			 drawKlient(str_header, curnum, goodscount);
			 lcd_hidecursor();
			 } else if (res == ANNULPOS)
			 return;
			 else if (res == ANNULR)
			 return;	/**/
			break;

		case KEY_C4:
			if ((curnum < goodscount))
			{
				while (1)
				{
					wait_start();
					PrintKlient(curnum); //����������� ������� ������� ������
					if (PrintFinal(1) == READY)
						break;
				}
				drawKlient(str_header, curnum, goodscount);
				break;

				case KEY_X:
				while (1)
				{
					wait_start();
					PrintAllKlients(goodscount);
					if (PrintFinal(1) == READY)
						break;
				}
				if ((curnum < goodscount))
				{
					drawKlient(str_header, curnum, goodscount);
				}
				else
				{
					drawKlient(str_header, curnum - 1, goodscount);
					//lcd_cl_row(1);
					glcd_print(">��������", 1);
				}
				break;

				case KEY_CLEAR:
				//�������� �������
				 if ((curnum!=goodscount)) {
					 res = NO_ACTION;
				 glcd_cls();
				 glcd_print("������� �������?", 1);
					  //�������� ������� ������ ����
						 for (;;) {
				 key.value = osMessageGet(kbdQueueHandle, osWaitForever).value.v;
				 switch (key.key) {
				 case KEY_TOTAL: //������ ����
				 res=READY;
				 break;
				 case KEY_ANNUL:
				 res= ANNULPOS;
				 break;
				 }
				 if (res!=NO_ACTION)
				 {
				 break;
				 }
				 }

				 if (res==READY)
				 {
				 uint8_t l= DeleteKlient(curnum);
				 er = memory_i_ReadKlientCount(&goodscount);
				 if (curnum>0)
				 curnum--;
				 }
				 drawKlient(str_header, curnum, goodscount);
				 }
				break;
			}
		}
	}
	return;
}



/*
 * ����� ������ ��� ������  ������ �������
 *  � ������ ����������������
 * */
void ShowListGoods() {

	uint8_t res = NO_ACTION;
	char numberstring[14];
	int curnum = 0; //���������� ����� �������� � ������
	drawnodeitem(list_departs, "������", curnum);
	lcd_hidecursor();
	key_pressed key;

	for (;;) {
		key.value = osMessageGet(kbdQueueHandle, osWaitForever).value.v;
		switch (key.key) {
		case KEY_DISCOUNT:		//up
			if (curnum - 1 >= 0) {
				curnum--;
				drawnodeitem(list_departs, "������", curnum);
				lcd_hidecursor();
			}
			break;
		case KEY_EXTRA:		//down
			if (curnum + 1 < list_departs->size) {
				curnum++;
				drawnodeitem(list_departs, "������", curnum);
				lcd_hidecursor();
			}
			break;
		case KEY_ANNUL:
			return;
			break;
		case KEY_MODE:
			glcd_cls();
			SP = 0;
			curmenu = (menuitem*) &topmenu;
			MySession.state &= ~(show_clock);
			//����� �������� ����
			drawmenu(curmenu, 0);
			selectmenu(curmenu, menu_reset, 0);
			return;
			break;
		case KEY_TOTAL:
			res = ShowListGoodsByDeps(curnum + 1);
			if (res == GOTO_R)
				return;
			drawnodeitem(list_departs, "������", curnum);
			break;
		}
	}
}

/*
 * �����  ������ ������� � ������ ����������������
 * numdep - ����� ������ (� �������)
 * ������������ ��������:
 * PREV - ����� (������� ��)
 * GOTO_R - ������� � ������ ������ (������� �)
 * */
uint8_t ShowListGoodsByDeps(uint8_t numdep) {

	uint8_t res = NO_ACTION;
	char numberstring[14];
	char * str_header = "������/������";
	uint16_t goodscount = 0;
	error_t er = memory_i_ReadGoodsCount(&goodscount,MEMORY_GOOD_COUNT_DEP_STARTADDRESS((numdep-1)));
	int curnum = 0; //���������� ����� �������� � ������
	drawGood(str_header, curnum, goodscount, numdep);
	lcd_hidecursor();
	key_pressed key;
	bool print_success = false;

	kbd_input_cur_mode = modeKIR_UP;

	for (;;) {
		key.value = osMessageGet(kbdQueueHandle, osWaitForever).value.v;
		switch (key.key) {
		case KEY_DISCOUNT:		//up
		if (curnum - 1 >= 0) {
			curnum--;
			drawGood(str_header, curnum, goodscount, numdep);
			lcd_hidecursor();
		}
		break;
	case KEY_EXTRA:		//down
		if (curnum + 1 < goodscount) {
			curnum++;
			drawGood(str_header, curnum, goodscount,numdep);
			lcd_hidecursor();
		}
		else if (curnum + 1== goodscount)
		{
			if ((goodscount<MAX_GOODS_DEP_COUNT))
										{//�������� ����� "��������"
										curnum++;
										//lcd_cl_row(1);
										glcd_print(">��������", 1);
										}
		}

		break;
	case KEY_ANNUL: // ���������� �������������� ������ � ������� ���������
		return PREV;
		break;
	case KEY_MODE:
		glcd_cls();
					SP = 0;
					curmenu = (menuitem*) &topmenu;
					MySession.state &= ~(show_clock);
					//����� �������� ����
				drawmenu(curmenu, 0);
				selectmenu(curmenu, menu_reset, 0);
		return GOTO_R;
		break;
	case KEY_TOTAL:
		if ((curnum==goodscount)) {
							res=NO_ACTION;
								glcd_cls();
								glcd_print("�������� �����?", 1);
								//�������� ������� ������ ����
												for (;;) {
													key.value = osMessageGet(kbdQueueHandle, osWaitForever).value.v;
													switch (key.key) {
													case KEY_TOTAL: //������ ����
														res=READY;
														break;
													case KEY_ANNUL:
														res= ANNULPOS;
														break;
													}
													if (res!=NO_ACTION)
													{
														break;
													}
												}

								if (res==READY) {
									uint16_t newgoodpos;
								//������� ��������� �� ����������  �� ��������� //�������� ��������� � ������
								res=	ShowCreateNodeGood(numdep, &newgoodpos);
									switch (res) {
										case NO_CHANGE:
											drawGood(str_header, curnum-1, goodscount, numdep);
											//lcd_cl_row(1);
											glcd_print(">��������", 1);
											break;
										case CHANGE_ALL:
											er = memory_i_ReadGoodsCount(&goodscount, MEMORY_GOOD_COUNT_DEP_STARTADDRESS((numdep-1)));
											curnum= (int) newgoodpos;
											drawGood(str_header, curnum, goodscount, numdep);
											break;
										case GOTO_R:
											glcd_cls();
																SP = 0;
																curmenu = (menuitem*) &topmenu;
																MySession.state &= ~(show_clock);
																//����� �������� ����
															drawmenu(curmenu, 0);
															selectmenu(curmenu, menu_reset, 0);
													return GOTO_R;
											break;
										}

								}
								else if (res==ANNULPOS)
								{
									drawGood(str_header, curnum-1, goodscount,numdep);
									//lcd_cl_row(1);
									glcd_print(">��������", 1);
									break;
								}
			}
		else {
		//������� � �������������� ����������/���������� �������� ������
					res= ShowNodeGood(curnum, numdep);
					switch (res) {
					case NO_CHANGE:
						break;
					case CHANGE_ALL:
						break;
					case GOTO_R:
					glcd_cls();
					SP = 0;
					curmenu = (menuitem*) &topmenu;
					MySession.state &= ~(show_clock);
					//����� �������� ����
					drawmenu(curmenu, 0);
					selectmenu(curmenu, menu_reset, 0);
					return GOTO_R;
						break;
					}
					drawGood(str_header, curnum, goodscount,numdep);//drawnodeitem(list, title, curnum);
		}
		break;

		// ���� ������ ������
	case KEY_0:
	case KEY_1:
	case KEY_2:
	case KEY_3:
	case KEY_4:
	case KEY_5:
	case KEY_6:
	case KEY_7:
	case KEY_8:
	case KEY_9:
	case KEY_00:
	case KEY_dot:

		res = str_searchInput(paramitemstr, key.key);
		if (res == NO_ACTION) {
			drawGood(str_header, curnum, goodscount,numdep);
			lcd_hidecursor();
		} else if (res == READY) {
			char c = paramitemstr[0];

			curnum = binarysearch_first_symbol(c, goodscount, numdep-1);
			drawGood(str_header, curnum, goodscount,numdep); //drawnodeitem_pos(list_goods, str_header, curnum,k);
			lcd_hidecursor();
		} else if (res == ANNULPOS)
			return PREV;
		else if (res == ANNULR)
			return GOTO_R;
		break;

	case KEY_C4:
if ((curnum<goodscount)) {
	while (1) {
			wait_start();
	PrintGood(curnum,  numdep); //����������� ������� ������� ������
				if (PrintFinal(1) == READY)
					break;
			}
				drawGood(str_header, curnum, goodscount,numdep);
}
			break;

	case KEY_X:
		while (1) {
				wait_start();
				PrintAllGoods(numdep, goodscount);
					if (PrintFinal(1) == READY)
						break;
				}
		 				if ((curnum<goodscount)) {
		 				drawGood(str_header, curnum, goodscount,numdep);
		 				}
		 				else
		 				{drawGood(str_header, curnum-1, goodscount, numdep);
						//lcd_cl_row(1);
						glcd_print(">��������", 1);
		 				}
		break;

	case KEY_CLEAR: //�������� �������� ������
		if ((curnum!=goodscount)) {
			res = NO_ACTION;
			glcd_cls();
			glcd_print("������� �������?", 1);
		//�������� ������� ������ ����
			for (;;) {
				key.value = osMessageGet(kbdQueueHandle, osWaitForever).value.v;
				switch (key.key) {
				case KEY_TOTAL: //������ ����
					res=READY;
					break;
				case KEY_ANNUL:
					res= ANNULPOS;
					break;
				}
				if (res!=NO_ACTION)
				{
					break;
				}
			}
			if (res==READY)
			{
				uint8_t l= DeleteGood(curnum,(numdep-1) );
				er = memory_i_ReadGoodsCount(&goodscount,MEMORY_GOOD_COUNT_DEP_STARTADDRESS((numdep-1)));
				if (curnum>0)
				curnum--;
			}

			drawGood(str_header, curnum, goodscount,numdep);
		}
		break;
		}
	}
	return PREV;
}




/*
 * �����  ������ ������ � ������ �����������
 * (������ � ������� ������� k)
 * index - ����� ������ (�������� � �������� �� �����)
 * numdep - ����� ������ (� �������)
 */
void reg_drawGood(char *str_header, uint16_t  index,  uint8_t k, uint16_t listsize, uint8_t numdep) {
		// ������� ������
	glcd_cls();
	//���������
	glcd_print(str_header, 0);

	if (listsize==0)
		{
		glcd_print("������� ���", 1);
		}
		else {

	//������� � ����������
	if (listsize > (1 + index))
		glcd_put_at(ARROW_RIGHT, 0, 19);
	char buf[4];
	sprintf(buf,"%d",index+1);
	glcd_print_at(buf, 0,19-strlen(buf));
	//lcd_putat((char) (((int) '0' + 1) + curnum) , 0,14);

	if (index > 0)
		glcd_put_at(ARROW_LEFT, 0, 18-strlen(buf));

	char strmas[4];
	sprintf(strmas, "%d>", k);
	glcd_print(strmas, 1);
	//����
	struct memory_good mygood;
	GetGoodByNum(index, numdep-1 , &mygood);

	glcd_print_at((char *) (mygood.name), 1, strlen(strmas));
		}
}


/*
 * �����  ������ ������� � ������ �����������, ����� ������
 * k - ����� ������� �������
 * */
uint8_t reg_ShowListGoods(char *str_header, uint8_t k,
		struct memory_good *mygood) {

	uint8_t res = NO_ACTION;
	char numberstring[14];
	int curnum = 0; //���������� ����� �������� � ������
	drawnodeitem(list_departs, str_header, curnum);
	lcd_hidecursor();
	key_pressed key;

	for (;;) {
		key.value = osMessageGet(kbdQueueHandle, osWaitForever).value.v;
		switch (key.key) {
		case KEY_X:
			if (k > 1)
				return REPEAT;
			break;
		case KEY_DISCOUNT:		//up
			if (curnum - 1 >= 0) {
				curnum--;
				drawnodeitem(list_departs, str_header, curnum);
				lcd_hidecursor();
			}
			break;
		case KEY_EXTRA:		//down
			if (curnum + 1 < list_departs->size) {
				curnum++;
				drawnodeitem(list_departs, str_header, curnum);
				lcd_hidecursor();
			}
			break;
		case KEY_ANNUL:
			return ANNULPOS;
			break;
		case KEY_MODE:
			glcd_cls();
			SP = 0;
			curmenu = (menuitem*) &topmenu;
			MySession.state &= ~(show_clock);
			//����� �������� ����
			drawmenu(curmenu, 0);
			selectmenu(curmenu, menu_reset, 0);
			return ANNULR;
			break;
		case KEY_SUBTOTAL: //����� ������, ����� �������

			res = reg_ShowListGoodsByDeps(curnum + 1, k, mygood);
			if (res == READY) {
				return READY;
			} else if (res == ANNULR)
				return ANNULR;
			else if (res == ANNULPOS)
				drawnodeitem(list_departs, str_header, curnum);
			break;
		case KEY_TOTAL:
			if (k > 1) {
				return READYR;
			}
			break;
		}
	}
}

/*
 * �����  ������ ������� � ������ �����������
 * k - ���������� ����� ����������� �������
 * numdep - ����� ������ (� �������)
 * */
uint8_t reg_ShowListGoodsByDeps(uint8_t numdep, uint8_t k,
		struct memory_good *mygood) {

	uint8_t res = NO_ACTION;
	char numberstring[14];
	char * str_header = "������/������";
	uint16_t goodscount = 0;
	error_t er = memory_i_ReadGoodsCount(&goodscount,
			MEMORY_GOOD_COUNT_DEP_STARTADDRESS((numdep - 1)));
	int curnum = 0; //���������� ����� �������� � ������
	reg_drawGood(str_header, curnum, k, goodscount, numdep);
	lcd_hidecursor();
	key_pressed key;

	kbd_input_cur_mode = modeKIR_UP;

	for (;;) {
		key.value = osMessageGet(kbdQueueHandle, osWaitForever).value.v;
		switch (key.key) {
		case KEY_DISCOUNT:		//up
			if (curnum - 1 >= 0) {
				curnum--;
				reg_drawGood(str_header, curnum, k, goodscount, numdep);
				lcd_hidecursor();
			}
			break;
		case KEY_EXTRA:		//down
			if (curnum + 1 < goodscount) {
				curnum++;
				reg_drawGood(str_header, curnum, k, goodscount, numdep);
				lcd_hidecursor();
			}
			break;
		case KEY_ANNUL:
			return ANNULPOS;
			break;
		case KEY_MODE:
			return ANNULR;
			break;

		case KEY_SUBTOTAL:
//����������� ����� � mygood
			GetGoodByNum(curnum, numdep - 1, mygood);
			return READY;
			break;

			// ���� ������ ������
		case KEY_0:
		case KEY_1:
		case KEY_2:
		case KEY_3:
		case KEY_4:
		case KEY_5:
		case KEY_6:
		case KEY_7:
		case KEY_8:
		case KEY_9:
		case KEY_00:
		case KEY_dot:

			res = str_searchInput(paramitemstr, key.key);
			if (res == NO_ACTION) {
				reg_drawGood(str_header, curnum, k, goodscount, numdep);
				lcd_hidecursor();
			} else if (res == READY) {
				char c = paramitemstr[0];
				curnum = binarysearch_first_symbol(c, goodscount, numdep - 1);
				reg_drawGood(str_header, curnum, k, goodscount, numdep);//drawnodeitem_pos(list_goods, str_header, curnum,k);
				lcd_hidecursor();
			} else if (res == ANNULPOS)
				return ANNULPOS;
			else if (res == ANNULR)
				return ANNULR;
			break;

		}
	}
}




/*
 * ������������� �������� ������ �������������
 */
void InitUser(char *name, char * pass, uint8_t rights, char *usertitle, int i) {

//�������� ��������

	param_table_item params[] =
	{
		{ paramtype_char, MEMORY_USER_NAMESIZE+1, 1, "���", NULL },
		{ paramtype_char, MEMORY_USER_PASSSIZE+1, 1, "������", NULL },
		{ paramtype_ticks, 7, 1, "����� �������", (void*) osAllocMem(sizeof(user_rights)) },
	};

	param_table_item *value = (param_table_item*) osAllocMem(sizeof(param_table_item) * (sizeof(params)/sizeof(params[0])));

	params[0].paramitem = strdup(name);

	params[1].paramitem = strdup(pass);

	uint8_t mask=1;
	for (int i = 0; i < 7; i++) {
		if (mask & rights) {
			user_rights[i].value = 1;
		} else {
			user_rights[i].value = 0;
		}
		mask=mask<<1;
	}

	memcpy((uint8_t *) (params[2].paramitem), user_rights,
			sizeof(user_rights));


	memcpy(value, params, sizeof(params));

	pushBack(list_users, createNodeItem(user, usertitle, value));
	menu_users[i].title = strdup(usertitle);
}

/*
 * ������������� �������� ������ ��������
 */
uint8_t InitKlient(char *name, char *id,uint8_t dics_num, uint32_t disc_proc,uint32_t ext_proc,
		uint64_t disc_val, uint64_t ext_val, uint64_t sum)
{
		//�������� ������
		struct memory_klient myklient;
		strcpy(myklient.name, name);
		strcpy(myklient.id_reader, id);
		//myklient.code=code;
		myklient.dics_num=dics_num;
		myklient.disc_proc=disc_proc;
		myklient.ext_proc=ext_proc;
		myklient.disc_val=disc_val;
		myklient.ext_val=ext_val;
		myklient.sum=sum;

		uint16_t pos;
	uint8_t res= 	InsertKlient(myklient, &pos);

	return res;
}


/*
 * ������������� �������� ������ �������
 */
uint8_t InitGood(char *name, uint32_t code, char *barcode, int depart, double price, uint8_t flag, uint8_t flag_service)
{
		uint16_t pos;
	//�������� ������
		struct memory_good mygood;
		strcpy(mygood.name, name);
		mygood.idcode=code;
		strcpy(mygood.barcode, barcode);
		mygood.secnum=(uint8_t)depart;

		uint64_t pr = DoubleToInt(price * 100);
		mygood.price=pr;
		mygood.drob=flag;
		mygood.service=flag_service;


	uint8_t res= 	InsertGood(mygood, &pos);

	return res;
}

/*
 * ������������� �������� ������ �������
 */
void InitDepart(struct memory_depart *mydep) {

	//�������� ��������
	struct _depart_info
	{
		uint8_t fix_code;
		//uint8_t service;
	} *pdepart_info;
	pdepart_info = (struct _depart_info*) osAllocMem(sizeof(struct _depart_info));

	//param_tick_item service[1] = { { "��", mydep->service } };

	param_table_item params[] = {
			{ paramtype_char, MEMORY_DEPART_NAMESIZE, 1, "��������", NULL },
			{ paramtype_int1, 1,	1, "��� ������", (void *) &pdepart_info->fix_code },
			//{ paramtype_ticks, 1,	1, "������", (void *) &pdepart_info->service },
	};
	param_table_item *value = (param_table_item*) osAllocMem(sizeof(param_table_item) * (sizeof(params)/sizeof(params[0])));

	params[0].paramitem =strdup(mydep->name);

	memcpy((uint8_t *) (params[1].paramitem), &(mydep->fixcode), sizeof(uint8_t));
//	memcpy((uint8_t *) (params[2].paramitem), service, sizeof(service));

	memcpy(value, params, sizeof(params));

	pushBack(list_departs, createNodeItem(depart,mydep->title, value));
}
/*
 * �������������� ������ � ������ (�������� ������)
 * depnum - ����� ������ (�� 0 �� 15)
 */
void EditDepartName(char *name, uint8_t depnum)
{	Node *node = getNthq(list_departs, depnum);
	NodeItem *nodeitem = (NodeItem *) node->value;
	int curnum=0; //��������� ����� ��������� (0 - ��� ��������)
	param_table_item *paramnodeitem = (param_table_item *) (nodeitem->value
				+ curnum);
	clearmas();
	strcpy(paramitemstr, name);
	editparamitem(nodeitem, curnum);
}


/*
 * �������������� ������ � ������ (��������, ��� ���, ��� ������, ��)
 * depnum - ����� ������ (�� 0 �� 15)
 */
void EditDepart(char *name, uint8_t fix_type, uint8_t code, uint8_t pr, uint8_t depnum)
{	Node *node = getNthq(list_departs, depnum);
	NodeItem *nodeitem = (NodeItem *) node->value;
	int curnum=0; //��������� ����� ��������� (0 - ��� ��������)
	clearmas();
	strcpy(paramitemstr, name);
	editparamitem(nodeitem, curnum);

	curnum=1; //��������� ����� ��������� (1 - ���)
	int numval=1;
/*	switch (fix_type)
	{
	case FixGeneral:
		numval=0;
		break;
	case FixSimpleIncom:
		numval=1;
		break;
	case FixSimpleIncomMinOutcom:
		numval=2;
		break;
	case FixSingleIncom:
		numval=3;
		break;
	case FixSingleRural:
		numval=4;
		break;
	case FixPatent:
		numval=5;
		break;
	}*/
	editparamitem_ticks_list(nodeitem, curnum, numval);

	curnum=2; //��������� ����� ��������� (2 - ��� ������)
	clearmas();
	sprintf(paramitemstr, "%d",code);
	editparamitem(nodeitem, curnum);

	curnum=3; //��������� ����� ��������� (3 - ������� �������)
	numval=(int)pr;
	editparamitem_ticks_list(nodeitem, curnum, numval);

}

/*
 * ������������� �������� ������ �������
 */
void InitFix(struct memory_fix *myfix) {
	param_tick_item incl[1] = { { "��", myfix->type } };
	param_table_item params[] =
	{
			{ paramtype_char, MEMORY_FIX_NAMESIZE, 0, "��������", (void *) (osAllocMem(sizeof(char) * (MEMORY_FIX_NAMESIZE+1))) },
			{ paramtype_int1, 1, 0, "��� ������", (void *) (osAllocMem(sizeof(uint8_t))) },
			{ paramtype_int1, 3, 1, "������ � %", (void *) (osAllocMem(sizeof(uint8_t))) },
			{ paramtype_ticks, 1, 1, "����������", (void *) (osAllocMem(sizeof(incl))) }
	};

	param_table_item *value = (param_table_item*) osAllocMem(sizeof(param_table_item) * (sizeof(params)/sizeof(params[0])));
	strcpy(params[0].paramitem, myfix->name);
	memcpy((uint8_t *) (params[1].paramitem), &(myfix->fixcode), sizeof(uint8_t));
	memcpy((uint8_t *) (params[2].paramitem), &(myfix->value), sizeof(uint8_t));

	memcpy((uint8_t *) (params[3].paramitem), incl, sizeof(incl));

	memcpy(value, params, sizeof(params));

	pushBack(list_fixes, createNodeItem(fix, myfix->title, value));
}

/*
 * ������������� �������� ������ ������/�������
 */
void InitDiscount(struct memory_discounts * mydisc) {
	param_tick_item discount_type[2] = { { "������", 1 }, { "�������", 0 } };

	param_table_item params[] =
	{
			{ paramtype_char, MEMORY_DISC_NAMESIZE, 1, "��������", (void *) (osAllocMem(sizeof(char) * 25)) },
			{ paramtype_list, 2, 0, "���", (void *) (osAllocMem(sizeof(discount_type))) }, //(0 - ������, 1 - �������)
			{ paramtype_int4, 3, 1, "������ � %", (void *) (osAllocMem(sizeof(uint32_t))) }, }; //TODO ����� ������������ ��������?

	param_table_item *value = (param_table_item*) osAllocMem(
			sizeof(param_table_item) * (sizeof(params)/sizeof(params[0])));

	strcpy(params[0].paramitem, mydisc->name);
	for (int i = 0; i < sizeof(discount_type) / sizeof(discount_type[0]); i++) {
		if (i == mydisc->type) {
			discount_type[i].value = 1;
		} else {
			discount_type[i].value = 0;
		}
	}

	memcpy((uint8_t *) (params[1].paramitem), discount_type,
			sizeof(discount_type));
	memcpy((uint32_t *) (params[2].paramitem), &(mydisc->value), sizeof(uint32_t));

	memcpy(value, params, sizeof(params));

	pushBack(list_discounts, createNodeItem(discount, mydisc->title, value));
}

/*
 * ������������� �������� ������ �������� ������
 */
void InitCashType(struct memory_cashtypes * mycashtype) {

	param_table_item params[] = {
				{paramtype_char, MEMORY_CASH_NAMESIZE, 1, "������������",
						(void *) (osAllocMem(sizeof(char) * (MEMORY_CASH_NAMESIZE+1)))  }
		};

		uint16_t num_params= sizeof(params) / sizeof(params[0]);

		param_table_item *value = (param_table_item*) osAllocMem(
				sizeof(param_table_item) * num_params);

	strcpy(params[0].paramitem, mycashtype->name);

	memcpy(value, params, sizeof(params));

	pushBack(list_cashtypes, createNodeItem(cash_types, mycashtype->title, value));
}


/*
 * ������������� �������� ������ ��������� �����
 * place - ��������� �� ���� ��� ������
 * flag - ������
 */
void InitAdvs(struct memory_advs * myadv) {
	param_tick_item print[1] = { { "��", myadv->print_flag } };
	param_tick_item places[2] = { { "����", 1 }, { "���", 0 } };

	param_table_item params[] = {
			{ paramtype_char, MEMORY_ADV_TEXTSIZE+1, 1, "�����", (void *) (osAllocMem(sizeof(char) * (MEMORY_ADV_TEXTSIZE+1))) },
			{ paramtype_list, 2, 1, "���������", (void *) (osAllocMem(sizeof(places))) },
			{ paramtype_ticks, 1, 1, "������", (void *) (osAllocMem(sizeof(print))) }, };

	param_table_item *value = (param_table_item*) osAllocMem(sizeof(param_table_item) * (sizeof(params)/sizeof(params[0])));

	strcpy(params[0].paramitem, myadv->text);
	for (int i = 0; i < sizeof(places) / sizeof(places[0]); i++) {
			if (i == myadv->place) {
				places[i].value = 1;
			} else {
				places[i].value = 0;
			}
		}
	memcpy((uint8_t *) (params[1].paramitem), places, sizeof(places));
	memcpy((uint8_t *) (params[2].paramitem), print, sizeof(print));

	memcpy(value, params, sizeof(params));

	pushBack(list_advs, createNodeItem(adv, myadv->title, value));
}


char *get_cash_type_name(uint8_t cashtype)
{
	char *p;
	if (cashtype < list_cashtypes->size)
	{
		Node * node;
		node = getNthq(list_cashtypes, cashtype);
		p =
				((char *) ((param_table_item *) ((param_table_item *) (node->value->value)
						+ 0)->paramitem));
	}
	else
	{
		p = str_no_cash_type;
	}
	return p;
}

static int CheckDateTime(RTC_DateTypeDef sDate, RTC_TimeTypeDef sTime)
{
return 1;
}

void clearmas() {
	memset(paramitemstr, 0, sizeof(paramitemstr));
}

void cutstr(char *str) {
	str[MAXCOLS - 2] = 0;
}
