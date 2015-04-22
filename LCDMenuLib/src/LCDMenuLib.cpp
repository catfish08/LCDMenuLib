/************************************************************************/
/*																		*/
/*								LCDMenuLib								*/
/*																		*/
/************************************************************************/
/* Autor:			Nils Feldk�mper										*/
/* Create:			03.02.2008											*/
/* Edit:			18.10.2014											*/
/************************************************************************/
/* License:			all Free											*/
/************************************************************************/

/************************************************************************/
/* Description:															*/
/* With this library, you can create menus with layers on base on the   */
/* Nested-Set-Model. For every menu element can be create a function    */
/* to control the content. This function is called automatical from the */
/* library and runs in a loop, without blocking other program parts.   */
/*																		*/
/* Support:  -- englischen Link einf�gen --                             */
/*                                                                      */
/************************************************************************/

/************************************************************************/
/* Description (german):												*/
/* Mit der Lib k�nnen LCD Men�s �ber mehrere Ebenen mit Hilfe des   	*/
/* Nested Set Models generiert werden. Jeder Men�punkt kann mit einer   */
/* Funktion hinterlegt werden die durch die Lib aufgerufen wird, sobald */
/* der Men�punkt aktiviert wird.										*/
/*																		*/
/* Support (german):	http://forum.arduino.cc/index.php?topic=73816.0	*/
/************************************************************************/

/************************************************************************/
/* Features:															*/
/* - max 254 menu elements												*/
/* - max 254 menu elements per layer								    */
/* - max 6 layers from root, configurable in LCDMenuLib.h				*/
/* - max support for 6 buttons up, down, left, right, back/quit, enter  */
/* - min 3 buttons needed up, down, enter                               */
/* - separation of structural and functional level                      */
/* - support for initscreen which is shown after x secounds or at begin */
/* - scrollbar when more menu elments in a layer then rows              */
/* - last cursor pos is saved											*/
/* - possibility to jump from one menu elment directly to another       */
/* - support for many different lcd librarys in LCDMenuLib___config.h   */
/* - 4bit lcd support													*/
/* - 8bit lcd support													*/
/* - i2c lcd support													*/
/* - shift register lcd support											*/
/* - DogLcd support														*/
/*																		*/
/* - many small function for other things								*/
/*																		*/
/* - no support for gaphic displays yet									*/
/************************************************************************/

#	include <LCDMenuLib.h>



/** 
 * constructor
 *
 * @params
 * @ - menu instance
 * @ - LCD instance
 * @ - flash table for menu elements
 * @ - lcd rows
 * @ - lcd colls
 * @return
 * 
 */
LCDMenuLib::LCDMenuLib(LCDMenu &p_r,_LCDML_lcd_type &p_d, const char * const *p_flash_table, const uint8_t p_rows, const uint8_t p_cols)
{
	// deklaration 
    rootMenu        = &p_r;
    curMenu         = rootMenu;
	flash_table     = p_flash_table;
    lcd             = &p_d;	
	// initialisation
	control			= 0; 
    cursor_pos      = 0;
    layer           = 0;
    layer_save[0]   = 0;
	child_cnt		= 0;
	rows			= p_rows;
	cols			= (p_cols-1); 	
    g_function      = _LCDMenuLib_NO_FUNC;
    button			= 0;
}

/** 
 * private function to search a menu elment
 *
 * @params
 * @ - menu instance
 * @ - menu element
 * @return
 * @ - uint8_t search status
 * 
 */
uint8_t LCDMenuLib::selectElementDirect(LCDMenu &p_m, uint8_t p_search)
{	
	LCDMenu * search = &p_m;
	LCDMenu * tmp;	
	uint8_t found = 0;
			
	do {
		//check elements for childs
		tmp=search->getChild(0, group);
		if(tmp) 
		{
			//press enter
			Button_enter();

			//search elements in this layer
			if(tmp->name == p_search) 
			{
				found = 1;				
				break;
			}		
			
			//nothing found

			//recursive search
			found = selectElementDirect(*tmp, p_search);
			
			//check result
			if(found == 1) 
			{
				//something found
				break; 
			} 
			else 
			{
				//nothing found
				//goto next root element
				Button_quit();
				Button_up_down_left_right(_LCDML_button_down);				
			}				
		} 
		else 
		{
			//no childs found
			
			//search
			if(search->name == p_search) 
			{
				//found something
				found = 1;				
				break;
			} 
			else 
			{
				//select next element			
				Button_up_down_left_right(_LCDML_button_down);
			}
		}		
	} while ((search=search->getSibling(1, group)) && found == 0);

	//return result
	return found;	
}



/*
 * public function goRoot
 * @param
 * @return
 */
void	LCDMenuLib::goRoot()
{
	//go back to root layer
	while(layer > 0) 
	{ 
		Button_quit(2); 
	}	
	//ends the last aktive function or do nothing
	Button_quit(2);
	
	//reset buttons and cursor position
	button = 0;
	cursor_pos = 0;
	curloc = 0;
	scroll = 0;
	

	display();


}

/*
 * public function jumpToElement
 * @param
 * @ - search element
 * @return
 */
void	LCDMenuLib::jumpToElement(uint8_t p_element)
{	
	goRoot();	
	
	//search element
	if(selectElementDirect(*rootMenu, p_element)) {
		//open this element
		Button_enter();
	}
}

/*
 * public function set cursor
 * @param
 * @return
 */
void	LCDMenuLib::setCursor()
{

	child_cnt = countChilds();

	//reset last cursor position
	if(cursor_pos > curloc-scroll) 
	{
		//reset cursor in row 0
        lcd->_LCDML_lcd_setCursor(0,cursor_pos);
        lcd->_LCDML_lcd_write(0x20);
    }
    else if(cursor_pos < curloc-scroll) 
	{
		//reset cursor at row > 0
		lcd->_LCDML_lcd_setCursor(0,curloc-scroll-1);
		lcd->_LCDML_lcd_write(0x20);
    }	
   
	//save current cursor position
	cursor_pos = curloc-scroll;

	if(cursor_pos > child_cnt) {
		cursor_pos = child_cnt;
	}
	

	//set cursor 
    lcd->_LCDML_lcd_setCursor(0,cursor_pos);
    lcd->_LCDML_lcd_write(_LCDMenuLib_cfg_cursor);




	//check scrollbar mode
	if(bitRead(control, _LCDMenuLib_control_scrollbar_l) == 1) 
	{
		//count elements in this layer
		uint8_t j = child_cnt;
	
		//show scrollbar
		if(j>(rows-1)) 
		{
			//reset row 0
			lcd->_LCDML_lcd_setCursor(cols, 0);
			lcd->_LCDML_lcd_write((uint8_t)0);			
			//reset row 1
			if(rows >= 2) 
			{				
				lcd->_LCDML_lcd_setCursor(cols, 1);
				lcd->_LCDML_lcd_write((uint8_t)0);
			}			
			//reset row 2
			if(rows >= 3) 
			{				
				lcd->_LCDML_lcd_setCursor(cols, 2);
				lcd->_LCDML_lcd_write((uint8_t)0);
			}			
			//reset row 3
			if(rows >= 4) 
			{				
				lcd->_LCDML_lcd_setCursor(cols, 3);
				lcd->_LCDML_lcd_write((uint8_t)0);
			}
          
			//show scrollbar
			if(curloc == 0) 
			{
				//first element
				lcd->_LCDML_lcd_setCursor(cols, 0);
				lcd->_LCDML_lcd_write((uint8_t)1);					
			} 
			else if(curloc == j) 
			{
				//last element
				lcd->_LCDML_lcd_setCursor(cols, (rows-1));
				lcd->_LCDML_lcd_write((uint8_t)4);			
			} 
			else 
			{
				//set scroll position
				uint8_t scroll_pos = ((4.*rows)/j*curloc);
				lcd->_LCDML_lcd_setCursor(cols, scroll_pos/4);			
				lcd->_LCDML_lcd_write((scroll_pos%4)+1);
			}
		}
	}	
}

/*
 * public function doScroll
 * @param
 * @return
 */
void	LCDMenuLib::doScroll()
{
	//only allow it to go up to menu element (one more if back button enabled)    
	while (curloc>0 && !curMenu->getChild(curloc, group))
	{			
		curloc--;
	}

	

		

	//scroll down
	if (curloc >= (rows+scroll)) 
	{
		scroll++;
		display();
	} 
	//scroll up
	else if (curloc < scroll) 
	{
		scroll--;
		display();
	} 
	//do not scroll
	else 
	{
		setCursor();
	}
	
}

/*
 * public function go back
 * @param
 * @return
 */
void	LCDMenuLib::goBack()
{
	//check button lock
	if(bitRead(control, _LCDMenuLib_control_menu_look) == 0) 
	{
		//check layer
		if(layer > 0) 
		{	
			//go back
			bitWrite(control, _LCDMenuLib_control_menu_back, 1);
			goMenu(*curMenu->getParent());
		}
	}	
}

/*
 * public function go back
 * @param
 * @return
 */
void	LCDMenuLib::goEnter()
{	
	//check button lock
	if(bitRead(control, _LCDMenuLib_control_menu_look) == 0) 
	{
		//declare opjects
		LCDMenu *tmp;
		tmp=curMenu;

		//check child		
		if ((tmp=tmp->getChild(curloc, group)))
		{			
			goMenu(*tmp);				
			curfuncname = tmp->name;			
		}		
	}
}

/*
 * public function go menu
 * @param
 * @ - menu object
 * @return
 */
void	LCDMenuLib::goMenu(LCDMenu &m)
{
	//declare variables
	int diff;
    scroll = 0;

	//set current menu object	
	curMenu=&m;	

	//check layer deep
	if(layer < _LCDMenuLib_cfg_cursor_deep) 
	{
		//check back button
		if(bitRead(control, _LCDMenuLib_control_menu_back) == 0) 
		{			
			//go into the next layer
			layer_save[layer] = curloc;
			layer++;		
			curloc = 0;
		} 
		else 
		{
			//button reset
			bitWrite(control, _LCDMenuLib_control_menu_back, 0);
			
			if(layer > 0) 
			{				
				layer--;				
				curloc = layer_save[layer];

				if(curloc >= rows) 
				{
					diff = curloc-(rows-1);
					for(int i=0; i<diff; i++) 
					{
						doScroll();
					}
				}
			}
		}
	}
	display();	
}

/*
 * private function to count the menu elements in this layer
 * @param
 * @return
 */
uint8_t    LCDMenuLib::countChilds()
{
	//declaration
	LCDMenu * tmp;
	int16_t j = 0;


	//check if element has childs
	if ((tmp = curMenu->getChild(0, group))) 
	{	
		while ((tmp = tmp->getSibling(1, group))) 
		{			
			j++;			
		}

		if (j < 0) 
		{
			j = 0;
		}		
    }

	return j;
}


/*
 * public function display menu
 * @param
 * @return
 */
void	LCDMenuLib::display()
{
	//declaration
    LCDMenu * tmp;
	LCDMenu * tmp_backup;
    uint8_t i = scroll;    
    uint8_t maxi=(rows+scroll);	
	char buffer[_LCDMenuLib_cfg_max_string_length];

	child_cnt = countChilds();
    
	//clear lcd
	lcd->_LCDML_lcd_clear();  

	//check children
    if ((tmp=curMenu->getChild(i, group))) 
	{
		//show menu structure
        do 
		{
			
			
				lcd->_LCDML_lcd_setCursor(0, i-scroll);
				lcd->_LCDML_lcd_write(0x20);
				lcd->_LCDML_lcd_setCursor(1, i-scroll);	

				//get name from flash table
				strcpy_P(buffer, (char*)pgm_read_word(&(flash_table[tmp->name])));				

				//check if string is to long for the lcd display with and without scrollbar
				if(bitRead(control, _LCDMenuLib_control_scrollbar_h) == 0 && bitRead(control, _LCDMenuLib_control_scrollbar_l) == 0) {
					//without scrollbar
					if(strlen(buffer) > (cols-1)) 
					{
						//error message
						lcd->_LCDML_lcd_print(F("too_long"));					
					}
					else 
					{
						//write string  to row
						lcd->_LCDML_lcd_print(buffer);
					}
				} else {
					//witch scrollbar
					if(strlen(buffer) > (cols-2)) 
					{
						//error message
						lcd->_LCDML_lcd_print(F("too_long"));
					} 
					else 
					{
						//write string to row
						lcd->_LCDML_lcd_print(buffer);
					}
				}
				//select next element in flash table
				i++;				
			
			
        } while ((tmp=tmp->getSibling(1, group)) && i<maxi); 

		

		//show scrollbar mode 2
		if(bitRead(control, _LCDMenuLib_control_scrollbar_h) == 1) 
		{
			//count menu elements
			uint8_t j = child_cnt;

			//show scrollbar
			if(j>(rows-1)) 
			{				
				if(bitRead(control, _LCDMenuLib_control_lcd_standard) == 0) 
				{
					if((curloc == j && curloc != 0) || (curloc != j && curloc != 0)) 
					{
						lcd->_LCDML_lcd_setCursor((cols),0);
						lcd->_LCDML_lcd_write((uint8_t)0);
					}
					if((curloc != j && curloc == 0) || (curloc != j && curloc != 0))
					{
						lcd->_LCDML_lcd_setCursor((cols),(rows-1));
						lcd->_LCDML_lcd_write((uint8_t)1);
					}
				}
				else if(bitRead(control, _LCDMenuLib_control_lcd_standard) == 1) 
				{
					if((curloc == j && curloc != 0) || (curloc != j && curloc != 0)) 
					{
						lcd->_LCDML_lcd_setCursor((cols),0);
						lcd->_LCDML_lcd_write((uint8_t)0x18);
						
					}
					if((curloc != j && curloc == 0) || (curloc != j && curloc != 0))
					{
						lcd->_LCDML_lcd_setCursor((cols),(rows-1));
						lcd->_LCDML_lcd_write((uint8_t)0x19);
					}
				}				
			}
		}        
    }
    else 
	{ // no children
        goBack();
        // function can run ...
    }
    setCursor();
}

/*
 * public function check Buttons
 * @param
 * @return
 * @ - uint8_t if a button is pressed
 */ 
uint8_t LCDMenuLib::checkButtons()
{
	//check if button is pressed
    if(bitRead(button, _LCDML_button)) 
	{
		//set button variable
		bitWrite(button, _LCDML_button, 0);		
        return true;
    }
    return false;    
}


 
/* 
 * public function check function end with button settings
 * @param 
 * - check parameter as bit mask
 * @return
 * - uint8_t status if function must end
 */
uint8_t	LCDMenuLib::checkFuncEnd(uint8_t check)
{
    //check mask   
    if(bitRead(control2, _LCDMenuLib_control2_endFuncOverExit)  || bitRead(check, 5) //direct
            || (((button & check) & (1<<_LCDML_button_enter)))  
            || (((button & check) & (1<<_LCDML_button_up)))
            || (((button & check) & (1<<_LCDML_button_down)))
            || (((button & check) & (1<<_LCDML_button_left)))
            || (((button & check) & (1<<_LCDML_button_right)))
    ) 
	{
		bitWrite(control2, _LCDMenuLib_control2_endFunc, 1);		

		//function ends
		Button_quit();
		return true;
    } 
    
    //function runs again
    return false;    
  }
  
  
/*
 * public function button enter 
 * @param
 * @return
 */ 
void	LCDMenuLib::Button_enter()
{
	
		if(g_function != _LCDMenuLib_NO_FUNC) 
		{
			//function is active
			bitWrite(button, _LCDML_button, 1);
			bitWrite(button, _LCDML_button_enter, 1);	
		} 
		
		//menu is active
		goEnter();
		g_function = curfuncname;
		
}


/*
 * public function button up down left right
 * @param
 * @ - but
 * @return
 */
void	LCDMenuLib::Button_up_down_left_right(uint8_t but)
{
	//check menu lock
	if(bitRead(control, _LCDMenuLib_control_menu_look) == 0)
	{		
		//enable up and down button for menu mode and scroll		
		switch(but)
		{
			case _LCDML_button_up:
				if(curloc > 0) 
				{
					curloc--;
				}				
			break;

			case _LCDML_button_down:
				if(curloc < child_cnt) {
					curloc++;
				}
			break;
		}
		doScroll();
	}
	
	bitWrite(button, but, 1);
	bitWrite(button, _LCDML_button, 1);	
}

/*
 * public function button quit 
 * @param
 * @ - opt => do not start the initscreen when the value is 2
 * @return
 */
void	LCDMenuLib::Button_quit(uint8_t opt)
{
	
			//no function active, do nothing
			if(g_function == _LCDMenuLib_NO_FUNC && layer == 0) 
			{					

			} 
			//function is active and have to close
			else 
			{

				if(bitRead(control2, _LCDMenuLib_control2_endFunc) || !bitRead(control, _LCDMenuLib_control_funcsetup) || opt == 2) 
				{
					bitWrite(control2, _LCDMenuLib_control2_endFuncOverExit, 0);
					bitWrite(control2, _LCDMenuLib_control2_endFunc, 0);
					
					//clear lcd / delete all output from the active function       
					lcd->_LCDML_lcd_clear();
		
					//display menu
					display();
  
					//go one layer back
					goBack();

					//set function variable to no function
					g_function = _LCDMenuLib_NO_FUNC;
				
					//delete function setup         
					bitWrite(control, _LCDMenuLib_control_funcsetup, 0);
    
					//enable menu control
					bitWrite(control, _LCDMenuLib_control_menu_look, 0);					
				} 
				else 
				{					
					bitWrite(control2, _LCDMenuLib_control2_endFuncOverExit, 1);		 
				}
			}		 

	button = 0;
}

/*
 * public function to init a new menu element function  (setup)
 * @param
 * @return
 * @ - status if this setup was run
 */
uint8_t	LCDMenuLib::FuncInit(void)
{
    //check setup bit    
	if(bitRead(control, _LCDMenuLib_control_funcsetup) == false) 
	{
		//enable function bit
		bitWrite(control, _LCDMenuLib_control_funcsetup, 1);
		
		//save function name for recall		
		g_function  = curfuncname;		
		
		//reset button status
		button = 0;
		
		//lock menu
		bitWrite(control, _LCDMenuLib_control_menu_look, 1);    
     
		return false;
	}	
    return true;    
} 
 
/*
 * public function getFunction Name
 * @param
 * @return
 * @ - return function name as number
 */ 
uint8_t	LCDMenuLib::getFunction()
{	
	return g_function;	
}

/*
 * public function to return the current function name
 * @param
 * @return
 * @ - current function name
 */
uint8_t	LCDMenuLib::getCurFunction()
{
	return curfuncname;
}

/*
 * public function to return the current layer position
 * @param
 * @return
 * @ - layer position
 */
uint8_t	LCDMenuLib::getLayer()
{
	return layer;
}


/*
 * public function timer for help function
 * @param
 * @ - timer variable
 * @ - waittime
 * @return
 * @ - status if time is resetted 
 */
uint8_t LCDMenuLib::Timer(unsigned long &p_var, unsigned long p_time)
{	
	if(p_var > millis()) 
	{  
		return false; 
	}
	p_var = millis() + p_time;
	return true;
}



uint8_t	LCDMenuLib::getCursorPos()
{
	return curloc; //return the current cursor position
}