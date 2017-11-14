/*! @file classicQBist.cpp

*  @brief This was originally published in c't issue 10-1995 by Dr. Jörn Loviscach, 
*  within the article - Ausgewürfelt - Moderne Kunst algorithmisch erzeugen.  The code 
*  produces algorthim based art.
*
*  @version 1.0.0
*
*  (C) Dr. Jörn Loviscach.
*
*  Licensed under either:
*  - Apache License, Version 2.0, http://www.apache.org/licenses/LICENSE-2.0  
*  - MIT license, http://opensource.org/licenses/MIT
*  at your option.
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*
*/
#include "stdafx.h"

// Original code from Dr. Jörn Loviscach
#define NUM_TRANSFORMS 36
#define NUM_REGISTERS 6
#define TOTAL_TRANSFORMS 9
#define PIXELS_PER_RUN 400

typedef struct { float x, y, z; } vector;
typedef void transform(vector * source, vector * control, vector * dest);
transform * (transformList[TOTAL_TRANSFORMS]);
short transformSequence[9][NUM_TRANSFORMS], source[9][NUM_TRANSFORMS],
 control[9][NUM_TRANSFORMS], dest[9][NUM_TRANSFORMS];
//vector reg[NUM_REGISTERS]; // Used by all 9 pictures!
short gx, gy, gVariation; // for asynchronous draw
short coarseness = 1; //fine
Point position[] =
 { {1, 1}, { 0,0 }, { 1,0 }, { 2,0 }, { 2,1 }, { 2,2 }, { 1,2 }, { 0,2 }, {0, 1}};
Boolean ready;
int pictWidth, pictHeight;

void projection (vector * source, vector * control, vector * dest) {
 float scalarProd;
     
 scalarProd = source->x * control->x
 + source->y * control->y + source->z * control->z;
 dest->x = scalarProd * source->x;
 dest->y = scalarProd * source->y;
 dest->z = scalarProd * source->z;
}
 
void shift (vector * source, vector * control, vector * dest) {
 dest->x = source->x + control->x;
 if (dest->x>= 1.0) dest->x -= 1.0;
 dest->y = source->y + control->y;
 if (dest->y>= 1.0) dest->y -= 1.0;
 dest->z = source->z + control->z;
 if (dest->z>= 1.0) dest->z -= 1.0;
}
 
void shiftBack (vector * source, vector * control, vector * dest) {
 dest->x = source->x-control->x;
 if (dest->x <= 0.0) dest->x += 1.0;
 dest->y = source->y-control->y;
 if (dest->y <= 0.0) dest->y += 1.0;
 dest->z = source->z-control->z;
 if (dest->z <= 0.0) dest->z += 1.0;
}
 
 void rotate (vector * source, vector * control, vector * dest) {
 dest->x = source->y;
 dest->y = source->z;
 dest->z = source->x;
}
 
void rotate2 (vector * source, vector * control, vector * dest) {
 dest->x = source->z;
 dest->y = source->x;
 dest->z = source->y;
}
 
void multiply (vector * source, vector * control, vector * dest) {
 dest->x = source->x * control->x;
 dest->y = source->y * control->y;
 dest->z = source->z * control->z;
}
 
void sine (vector * source, vector * control, vector * dest) {
 dest->x = 0.5f + 0.5f * sinf (20.0f * source->x * control->x);
 dest->y = 0.5f + 0.5f * sinf (20.0f * source->y * control->y);
 dest->z = 0.5f + 0.5f * sinf (20.0f * source->z * control->z);
}
 
void conditional (vector * source, vector * control, vector * dest) {
 if (control->x + control->y + control->z > 0.5) {
 dest->x = source->x;
 dest->y = source->y;
 dest->z = source->z;
 }
 else {
 dest->x = control->x;
 dest->y = control->y;
 dest->z = control->z;
 }
}
 
void complement (vector * source, vector * control, vector * dest) {
 dest->x = 1.0f-source->x;
 dest->y = 1.0f-source->y;
 dest->z = 1.0f-source->z;
}
 
void initTransforms (void) {
 transformList [0] = & projection;
 transformList [1] = & shift;
 transformList [2] = & shiftBack;
 transformList [3] = & rotate;
 transformList [4] = & rotate2;
 transformList [5] = & multiply;
 transformList [6] = & sine;
 transformList [7] = & conditional;
 transformList [8] = & complement;
}
void initBaseTransform(void) {
    int i;

	initTransforms();

    for(i=0;i<NUM_TRANSFORMS;i++) {
        transformSequence[0][i] = myRand()%TOTAL_TRANSFORMS;
        source[0][i] = myRand()%NUM_REGISTERS;
        control[0][i] = myRand()%NUM_REGISTERS;
        dest[0][i] = myRand()%NUM_REGISTERS;
    }
}

void makeVariations(void) {
    short i, k;
    
    for(k=1;k<=8;k++) {
        for(i=0;i<NUM_TRANSFORMS;i++) {
            transformSequence[k][i] = transformSequence[0][i];
            source[k][i]  = source[0][i];
            control[k][i] = control[0][i];
            dest[k][i]    = dest[0][i];
        }

        switch(coarseness) {
            case 3: //coarse
                transformSequence[k][myRand()%NUM_TRANSFORMS] =
                                            myRand()%TOTAL_TRANSFORMS;
                source[k][myRand()%NUM_TRANSFORMS] =
                                               myRand()%NUM_REGISTERS;
                control[k][myRand()%NUM_TRANSFORMS] =
                                               myRand()%NUM_REGISTERS;
                dest[k][myRand()%NUM_TRANSFORMS] =
                                               myRand()%NUM_REGISTERS;
                //Kein break!
            case 2: //medium
                transformSequence[k][myRand()%NUM_TRANSFORMS] =
                                            myRand()%TOTAL_TRANSFORMS;
                source[k][myRand()%NUM_TRANSFORMS] =
                                               myRand()%NUM_REGISTERS;
                control[k][myRand()%NUM_TRANSFORMS] =
                                               myRand()%NUM_REGISTERS;
                dest[k][myRand()%NUM_TRANSFORMS] =
                                               myRand()%NUM_REGISTERS;
                break;
            case 1: //fine
                switch(myRand()%4) {
                    case 0:
                       transformSequence[k][myRand()%NUM_TRANSFORMS] =
                                            myRand()%TOTAL_TRANSFORMS;
                        break;
                    case 1:
                        source[k][myRand()%NUM_TRANSFORMS] =
                                               myRand()%NUM_REGISTERS;
                        break;
                    case 2:
                        control[k][myRand()%NUM_TRANSFORMS] =
                                               myRand()%NUM_REGISTERS;
                        break;
                    case 3:
                        dest[k][myRand()%NUM_TRANSFORMS] =
                                               myRand()%NUM_REGISTERS;
                }
                break;
        }
    }
}

#if 0
void draw(Boolean fromStart) {  //asynchron: immer wieder aufrufen
    short i, j, k;              // Bei Bildänderung fromStart gesetzt.
    RGBColor myColor;
    Str255 myString;
    
    if(fromStart) {
        gx = gy = gVariation = 0;
        ready = false;
    }
    
    if(ready) return;
    
    SetPort(myWindow); //Mac
    
    for(k=0;k<PIXELS_PER_RUN;k++) {
        for(j=0;j<NUM_REGISTERS;j++) { //Register laden
            reg[j].x = ((float)gx)/(float)pictWidth;
            reg[j].y = ((float)gy)/(float)pictHeight;
            reg[j].z = ((float)j)/(float)NUM_REGISTERS;
        }
        
        for(i=0; i<NUM_TRANSFORMS; i++) {
            transformList[transformSequence[gVariation][i]]
                                     (&(reg[source [gVariation][i]]),
                                      &(reg[control[gVariation][i]]),
                                      &(reg[dest   [gVariation][i]]));
        }
        
        myColor.red   = (unsigned short)(0xFFFEU * reg[0].x);
        myColor.green = (unsigned short)(0xFFFEU * reg[0].y);
        myColor.blue  = (unsigned short)(0xFFFEU * reg[0].z);
        SetCPixel(position[gVariation].v*pictWidth +gx,
                 position[gVariation].h*pictHeight+gy,&myColor); //Mac

        gy++;
        if(gy == pictHeight) {
            gy = 0;
            gx++;
            if(gx == pictWidth) {
               gx=0; //Das fertige Teilbild mit einer Zahl beschriften
               MoveTo(position[gVariation].v*pictWidth +4,
                      position[gVariation].h*pictHeight+14); //Mac
               NumToString(gVariation,myString); //Mac
               DrawString(myString); //Mac
               gVariation++;
               if(gVariation==9) {
                   ready = true;
                   return;
                }
            }
        }
    }
}
#endif

// modified from the classic version above
void QBist(float x, float y, unsigned short *r, unsigned short *g, unsigned short *b ) 
{
	vector reg[NUM_REGISTERS];

	for (int j = 0; j<NUM_REGISTERS; j++) { //Register laden
		reg[j].x = x;
		reg[j].y = y;
		reg[j].z = ((float)j) / (float)NUM_REGISTERS;
	}

	for (int i = 0; i<NUM_TRANSFORMS; i++) {
		transformList[transformSequence[0][i]]
		(&(reg[source[0][i]]),
			&(reg[control[0][i]]),
			&(reg[dest[0][i]]));
	}

	*r = (unsigned short)(0xFFFF * reg[0].x);
	*g = (unsigned short)(0xFFFF * reg[0].y);
	*b = (unsigned short)(0xFFFF * reg[0].z);
}



void modifyQBistGenes(void)
{
	int same = 1;
	do
	{
		makeVariations();

		for (int i = 0; i < NUM_TRANSFORMS; i++)
		{
			if(transformSequence[0][i] != transformSequence[1][i])
				transformSequence[0][i] = transformSequence[1][i], same = 0;
			if (source[0][i] != source[1][i])
				source[0][i] = source[1][i], same = 0;
			if (control[0][i] != control[1][i])
				control[0][i] = control[1][i], same = 0;
			if (dest[0][i] != dest[1][i])
				dest[0][i] = dest[1][i], same = 0;
		}
	} while (same);
}
