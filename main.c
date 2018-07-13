#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#pragma pack(1)	// 構造体のメモリを詰める
// STL Header
typedef struct{
	unsigned char ucHeader[80];
	unsigned int  uiTriangleNum;

}T_Header;

// STL Data (single triangle)
typedef struct{
	float fNormalVector[3];
	float fCordinate[3][3];
	unsigned char ucUnused[2];
}T_Data;

// PCD Data (single point)
typedef struct{
	float fCordinate[3];
}T_PCD_Data;
#pragma pack()

void prinptData(T_Data ptData){
	printf("Normal Vector: ");
	for(int i=0; i<3; i++){
		printf("%.0f,",ptData.fNormalVector[i]);
	}
	printf("\n");

	printf("Cordinates:\n");
	for(int i=0; i<3; i++){
		printf("%d: ", i);
		for(int j=0; j<3; j++){
			printf("%.0f,",ptData.fCordinate[i][j]);
		}
		printf("\n");
	}
	printf("\n");
}

// 長さ
float calcLength(float fVector[3]){
	float fRet = 0;
	for(int i=0; i<3; i++){
		fRet += fVector[i]*fVector[i];
	}
	return sqrt(fRet);
}

// 外積
float calcArea(float fVectorA[3], float fVectorB[3]){
	float fRet[3]={0,0,0};
	for(int i=0; i<3; i++){
		fRet[i] += fVectorA[(i+1)%3]*fVectorB[(i+2)%3];
		fRet[i] -= fVectorA[(i+2)%3]*fVectorB[(i+1)%3];
	}
	return calcLength(fRet);
}

//void expPoint(float fVector[3]){
//	for(int i=0; i<3; i++){
//		printf("%.2f,",fVector[i]);
//	}
//	printf("\n");
//}

int main(int argc, char *argv[]){
	FILE* fp;
	T_Header tHeader;
	T_Data *ptData;
	float fA[3], fAUnit[3];
	float fB[3], fBUnit[3];
	float fSUnit = 100;
	float fDenLen;
	char ascFileName[50];
	unsigned int uiCountMax = 1000000;
	unsigned int uiCount = 0;
	T_PCD_Data *ptPCDData;

	if(argc != 2){
		printf("Error. Invalid argument.\n");
		printf("%s 'File Name' \n", argv[0]);
		return 0;
	}else{
		printf("Start STL2PCD Conversion.\n");
	}

	// Open STL file
	sprintf(ascFileName,"%s.stl",argv[1]);
	fp = fopen(ascFileName , "rb" );
	if( fp == NULL ){
		fputs( "File could not open.\n", stderr );
		exit( EXIT_FAILURE );
	}

	// Load Header
	if( fread(&tHeader, sizeof(tHeader), 1, fp ) < 1 ){
		fputs( "Header read error.\n", stderr );
		exit( EXIT_FAILURE );
	}

	// Load Data
	ptData = malloc(sizeof(T_Data) * tHeader.uiTriangleNum);
	if( fread(ptData, sizeof(T_Data), tHeader.uiTriangleNum, fp ) < 1 ){
		fputs( "Data read error.\n", stderr );
		exit( EXIT_FAILURE );
	}

//	printf("Header size: %d\n",(int)sizeof(tHeader));
//	printf("Single data size: %d\n",(int)sizeof(ptData));
	printf("Number of Triangles: %d\n\n",tHeader.uiTriangleNum);

	printf("Triangle Data\n");
	for(int i=0; i<tHeader.uiTriangleNum; i++){
		prinptData(ptData[i]);
	}

	fclose(fp);

	// Calculate Point cloud from STL
	ptPCDData = malloc(sizeof(T_PCD_Data) * uiCountMax);
	for(int i=0; i<tHeader.uiTriangleNum; i++){
		// Two vectors on Triangle plane
		for(int j=0; j<3; j++){
			fA[j] = ptData[i].fCordinate[1][j] - ptData[i].fCordinate[0][j];
			fB[j] = ptData[i].fCordinate[2][j] - ptData[i].fCordinate[0][j];
		}

		// Calculate Unit vectors
		for(int j=0; j<3; j++){
			fAUnit[j] = fA[j]/calcLength(fA);
			fBUnit[j] = fB[j]/calcLength(fB);
		}

		// Calculate Lattice points in Triangle
		fDenLen = sqrt(2*fSUnit/calcArea(fA,fB));
		for(float a=0; a <= calcLength(fA); a+=fDenLen){
			for(float b=0; b <= calcLength(fB); b+=fDenLen){
				// Inside triangle condition
				if(a/calcLength(fA) + b/calcLength(fB) <= 1){
					for(int j=0; j<3; j++){
						ptPCDData[uiCount].fCordinate[j] = ptData[i].fCordinate[0][j] + a * fAUnit[j] + b * fBUnit[j];
					}

					uiCount++;
					if(uiCount > uiCountMax){
						fputs( "Over max point amount\n", stderr );
						exit( EXIT_FAILURE );
					}
				}
			}
		}
	}

	// Export PCD
	FILE *fpExp;
	sprintf(ascFileName,"%s.pcd",argv[1]);
	fpExp = fopen(ascFileName, "wb");
	fprintf(fpExp, "# .PCD v.7 - Point Cloud Data file format\n");
	fprintf(fpExp, "VERSION .7\nFIELDS x y z\nSIZE 4 4 4\nTYPE F F F\nCOUNT 1 1 1\n");
	fprintf(fpExp, "WIDTH %u\nHEIGHT 1\nVIEWPOINT 0 0 0 1 0 0 0\nPOINTS %u\nDATA binary\n", uiCount, uiCount);
	fwrite(ptPCDData, sizeof(ptPCDData), uiCount, fpExp);
	fclose(fpExp);

	// Export ASCII
	sprintf(ascFileName,"%s.asc",argv[1]);
	fpExp = fopen(ascFileName, "wb");
	for(int i=0; i<uiCount; i++){
		fprintf(fpExp,"%f %f %f\n",ptPCDData[i].fCordinate[0], ptPCDData[i].fCordinate[1], ptPCDData[i].fCordinate[2]);
	}
	fclose(fpExp);

	free(ptData);
	free(ptPCDData);
}
