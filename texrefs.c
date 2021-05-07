/* texrefs.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* TeX Convertes: Label/Citation Bookkeeping                                 */
/*                                                                           */
/*****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "strutil.h"
#include "texutil.h"
#include "texrefs.h"

/*--------------------------------------------------------------------------*/

typedef struct sRefSave
{
  struct sRefSave *pNext;
  char *pRefName, *pValue;
} tRefSave, *tpRefSave;

/*--------------------------------------------------------------------------*/

static tpRefSave pFirstRefSave, pFirstCiteSave;

/*!------------------------------------------------------------------------
 * \fn     AddList(tpRefSave *ppList, const char *pDescr, const char *pName, const char *pValue)
 * \brief  add/update list
 * \param  ppList list root
 * \param  pDescr type of list contents
 * \param  pName name of label
 * \param  pValue value of label
 * ------------------------------------------------------------------------ */

static void AddList(tpRefSave *ppList, const char *pDescr, const char *pName, const char *pValue)
{
  tpRefSave pRun, pPrev, pNeu;
  int cmp = -1;
  char err[200];

  for (pRun = *ppList, pPrev = NULL; pRun; pPrev = pRun, pRun = pRun->pNext)
    if ((cmp = strcmp(pRun->pRefName, pName)) >= 0)
      break;

  if (pRun && !cmp)
  {
    if (strcmp(pRun->pValue, pValue))
    {
      as_snprintf(err, sizeof(err), "value of %s '%s' has changed", pDescr, pName);
      Warning(err);
      DoRepass = True;
      free(pRun->pValue);
      pRun->pValue = as_strdup(pValue);
    }
  }
  else
  {
    pNeu = (tpRefSave) malloc(sizeof(*pNeu));
    pNeu->pRefName = as_strdup(pName);
    pNeu->pValue = as_strdup(pValue);
    pNeu->pNext = pRun;
    if (!pPrev)
      *ppList = pNeu;
    else
      pPrev->pNext = pNeu;
  }
}

/*!------------------------------------------------------------------------
 * \fn     GetList(tpRefSave *pList, const char *pDescr, const char *pName, char *pDest)
 * \brief  retrieve value of label
 * \param  pList list to search
 * \param  pDescr type of list contents
 * \param  pName name of label
 * \param  pDest result buffer
 * ------------------------------------------------------------------------ */

static void GetList(tpRefSave pList, const char *pDescr, const char *pName, char *pDest)
{
  tpRefSave pRun;
  char err[200];

  for (pRun = pList; pRun; pRun = pRun->pNext)
    if (!strcmp(pName, pRun->pRefName))
      break;

  if (!pRun)
  {
    as_snprintf(err, sizeof(err), "undefined %s '%s'", pDescr, pName);
    Warning(err); DoRepass = True;
  }
  strcpy(pDest, !pRun ? "???" : pRun->pValue);
}

/*!------------------------------------------------------------------------
 * \fn     FreeList(tpRefSave *ppList)
 * \brief  free list of references
 * \param  ppList list to free
 * ------------------------------------------------------------------------ */

static void FreeList(tpRefSave *ppList)
{
  while (*ppList)
  {
    tpRefSave pOld = *ppList;
    *ppList = pOld->pNext;

    free(pOld->pValue);
    free(pOld->pRefName);
    free(pOld);
  }
}

/*!------------------------------------------------------------------------
 * \fn     PrintList(const char *pFileName, tpRefSave pList, const char *pDescr)
 * \brief  save list to file
 * \param  pFileName file to write label list to
 * \param  pList list to dump
 * \param  pDescr type of list contents
 * ------------------------------------------------------------------------ */

void PrintList(const char *pName, tpRefSave pList, const char *pDescr)
{
  tpRefSave pRun;
  FILE *pFile = fopen(pName, "a");

  if (!pFile)
    perror(pName);

  for (pRun = pList; pRun; pRun = pRun->pNext)
    fprintf(pFile, "%s %s %s\n", pDescr, pRun->pRefName, pRun->pValue);
  fclose(pFile);
}

/*!------------------------------------------------------------------------
 * \fn     InitLabels(void)
 * \brief  reset list of labels
 * ------------------------------------------------------------------------ */

void InitLabels(void)
{
  pFirstRefSave = NULL;
}

/*!------------------------------------------------------------------------
 * \fn     AddLabel(const char *pName, const char *pValue)
 * \brief  add/update label
 * \param  pName name of label
 * \param  pValue value of label
 * ------------------------------------------------------------------------ */

void AddLabel(const char *pName, const char *pValue)
{
  AddList(&pFirstRefSave, "label", pName, pValue);
}

/*!------------------------------------------------------------------------
 * \fn     GetLabel(const char *pName, char *pDest)
 * \brief  retrieve value of label
 * \param  pName name of label
 * \param  pDest result buffer
 * ------------------------------------------------------------------------ */

void GetLabel(const char *pName, char *pDest)
{
  GetList(pFirstRefSave, "label", pName, pDest);
}

/*!------------------------------------------------------------------------
 * \fn     PrintLabels(const char *pFileName)
 * \brief  save label list to file
 * \param  pFileName file to write label list to
 * ------------------------------------------------------------------------ */

void PrintLabels(const char *pFileName)
{
  PrintList(pFileName, pFirstRefSave, "Label");
}

/*!------------------------------------------------------------------------
 * \fn     FreeLabels(void)
 * \brief  free list of labels
 * ------------------------------------------------------------------------ */

void FreeLabels(void)
{
  FreeList(&pFirstRefSave);
}

/*!------------------------------------------------------------------------
 * \fn     InitCites(void)
 * \brief  reset list of citations
 * ------------------------------------------------------------------------ */

void InitCites(void)
{
  pFirstCiteSave = NULL;
}

/*!------------------------------------------------------------------------
 * \fn     AddCite(const char *pName, const char *pValue)
 * \brief  add/update citation
 * \param  pName name of citation
 * \param  pValue value of citation
 * ------------------------------------------------------------------------ */

void AddCite(const char *pName, const char *pValue)
{
  AddList(&pFirstCiteSave, "citation", pName, pValue);
}

/*!------------------------------------------------------------------------
 * \fn     GetCite(char *pName, char *pDest)
 * \brief  retrieve value of citation
 * \param  pName name of citation
 * \param  pDest result buffer
 * \return
 * ------------------------------------------------------------------------ */

void GetCite(char *pName, char *pDest)
{
  GetList(pFirstCiteSave, "citation", pName, pDest);
}

/*!------------------------------------------------------------------------
 * \fn     PrintCites(const char *pFileName)
 * \brief  save list of citations to file
 * \param  pFileName destination file name
 * ------------------------------------------------------------------------ */

void PrintCites(const char *pFileName)
{
  PrintList(pFileName, pFirstCiteSave, "Citation");
}

/*!------------------------------------------------------------------------
 * \fn     FreeCites(void)
 * \brief  free list of citations
 * ------------------------------------------------------------------------ */

void FreeCites(void)
{
  FreeList(&pFirstCiteSave);
}
