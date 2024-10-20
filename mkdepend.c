/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "strutil.h"
#include "stringlists.h"

#define ObjExtension ".o"

static StringRecPtr inc_path_list = NULL,
                    def_list = NULL;

static char *getobj(const char *pSrc)
{
  static char buffer[255];
  int l = strlen(pSrc), bm5 = sizeof(buffer) - 5;
  char *pSearch;

  if (l < bm5)
    l = bm5;
  memcpy(buffer, pSrc, l); buffer[l] = '\0';

  pSearch = strrchr(buffer, '.');
  if (pSearch)
    strcpy(pSearch, ObjExtension);

  return buffer;
}

int MayRecurse(const char *pFileName)
{
  int l = strlen(pFileName);

  /* .rsc files are autogenerated and do not contain any
     include statements - no need to scan them */

  if ((l > 4) && (!strcmp(pFileName + l - 4, ".rsc")))
    return 0;

  return 1;
}

static Boolean regard_sym(const char *p_name)
{
  return !strcmp(p_name, "_USE_MSH");
}

static Boolean def_present(const char *p_name)
{
  StringRec *p_run = def_list;

  for (p_run = def_list; p_run; p_run = p_run->Next)
  {
    if (!strcmp(p_name, p_run->Content))
      return True;
  }
  return False;
}

static Boolean curr_if = True;

typedef struct if_stack
{
  struct if_stack *p_next;
  Boolean condition, save_curr_if;
  Boolean evaluated, else_occured;
} if_stack_t;
static if_stack_t *p_if_stack_tos = NULL;

static void push_if(Boolean condition, Boolean evaluated)
{
  if_stack_t *p_new_if = (if_stack_t*)calloc(1, sizeof(*p_new_if));

  p_new_if->save_curr_if = curr_if;
  p_new_if->condition = condition;
  p_new_if->evaluated = evaluated;
  p_new_if->else_occured = False;
  p_new_if->p_next = p_if_stack_tos;
  p_if_stack_tos = p_new_if;
  curr_if = evaluated ? curr_if && condition : curr_if;
}

static void toggle_if(const char *p_file, int line)
{
  if (!p_if_stack_tos)
  {
    fprintf(stderr, "%s:%d: #else without #ifdef\n", p_file, line);
    exit(1);
  }
  else if (p_if_stack_tos->else_occured)
  {
    fprintf(stderr, "%s:%d: #ifdef with more than one #else\n", p_file, line);
    exit(1);
  }
  if (p_if_stack_tos->evaluated)
    curr_if = p_if_stack_tos->save_curr_if && !p_if_stack_tos->condition;
  p_if_stack_tos->else_occured = True;
}

static void pop_if(const char *p_file, int line)
{
  if_stack_t *p_old_if;

  if (!p_if_stack_tos)
  {
    fprintf(stderr, "%s:%d: #endif without #ifdef\n", p_file, line);
    exit(1);
  }
  curr_if = p_if_stack_tos->save_curr_if;
  p_old_if = p_if_stack_tos;
  p_if_stack_tos = p_old_if->p_next;
  free(p_old_if);
}

static void ParseFile(const char *pFileName, const char *pParentFileName, StringRecPtr *pFileList)
{
  FILE *pFile;
  int l, file_stat, line_num = 0;
  char line[512], *pCmd, *pName, *pValue;
  String raw_name, eff_name;
  struct stat stat_buf;

  pFile = fopen(pFileName, "r");
  if (!pFile)
  {
    if (pParentFileName)
      fprintf(stderr, "%s: ", pParentFileName);
    perror(pFileName);
    return;
  }

  while (!feof(pFile))
  {
    if (!fgets(line, sizeof(line), pFile))
      break;
    line_num++;
    l = strlen(line);
    if ((l > 0) && (line[l - 1] == '\n'))
      line[l - 1] = '\0';

    if (*line != '#')
      continue;
    pCmd = strtok(line + 1, " \t");

    if (!pCmd);
    else if (!strcmp(pCmd, "include") && curr_if)
    {
      pName = strtok(NULL, " \t");
      if (!pName)
        continue;
      l = strlen(pName);
      if ((*pName != '"') || (pName[l - 1] != '"'))
        continue;

      if (l - 1 < (int)sizeof(raw_name))
      {
        memcpy(raw_name, pName + 1, l - 2);
        raw_name[l - 2] = '\0';
        strmaxcpy(eff_name, raw_name, sizeof(eff_name));
        file_stat = stat(eff_name, &stat_buf);
        if (file_stat)
        {
          StringRecPtr p_run;
          const char *p_path;

          for (p_path = GetStringListFirst(inc_path_list, &p_run);
               p_path; p_path = GetStringListNext(&p_run))
          {
            size_t l = strlen(p_path);
            Boolean term_path = (l > 0) && ((p_path[l -1] == '/') || (p_path[l -1] == '\\'));
            as_snprintf(eff_name, sizeof(eff_name), "%s%s%s", p_path, term_path ? "" : "/", raw_name);
            file_stat = stat(eff_name, &stat_buf);
            if (!file_stat)
              break;
          }
        }
        /* If file is not present, it is created dynamically and expected to appear in the
           object subdirectory, which is passed in as -I path.  Keep last generated eff_name
           for the dependency list: */
        if (!StringListPresent(*pFileList, eff_name))
        {
          AddStringListLast(pFileList, eff_name);
          if (MayRecurse(eff_name))
            ParseFile(eff_name, pFileName, pFileList);
        }
      }
    }
    else if (!strcmp(pCmd, "if"))
    {
      push_if(True, False);
    }
    else if (!strcmp(pCmd, "ifdef"))
    {
      pName = strtok(NULL, " \t");
      if (!curr_if || !regard_sym(pName))
        push_if(True, False);
      else
        push_if(def_present(pName), True);
    }
    else if (!strcmp(pCmd, "ifndef"))
    {
      pName = strtok(NULL, " \t");
      if (!curr_if || !regard_sym(pName))
        push_if(True, False);
      else
        push_if(!def_present(pName), True);
    }
    else if (!strcmp(pCmd, "else"))
    {
      if (curr_if)
        toggle_if(pFileName, line_num);
    }
    else if (!strcmp(pCmd, "endif"))
    {
      pop_if(pFileName, line_num);
    }
    else if (!strcmp(pCmd, "elif"))
    {
      /* TODO */
    }
    else if (!strcmp(pCmd, "define"))
    {
      pName = strtok(NULL, " \t");
      pValue = strtok(NULL, " \t");

      if (strchr(pName, '(') || (pValue && *pValue))
        continue;
    }
    else if (!strcmp(pCmd, "undef"))
    {
      pName = strtok(NULL, " \t");
    }
  }
  fclose(pFile);
}

int main(int argc, char **argv)
{
  int z, ArgsAreObj = 0;
  FILE *pDepFile;
  const char *pDepFileName = NULL,
             *pOutFileName = NULL,
             *pObjExtension = NULL;
  char used[1024];
  StringRecPtr FileList, SrcList;

  if (argc < 2)
  {
    fprintf(stderr, "usage: %s [args] <file(s)>\n", *argv);
    exit(1);
  }

  memset(used, 0, sizeof(used));

  for (z = 1; z < argc; z++)
    if ((!used[z]) && (*argv[z] == '-'))
    {
      used[z] = 1;
      if (!strcmp(argv[z] + 1, "d"))
      {
        if (z >= argc - 1)
          pDepFileName = NULL;
        else
        {
          pDepFileName = argv[z + 1];
          used[z + 1] = 1;
        }
      }
      else if (!strcmp(argv[z] + 1, "o"))
      {
        if (z >= argc - 1)
          pOutFileName = NULL;
        else
        {
          pOutFileName = argv[z + 1];
          used[z + 1] = 1;
        }
      }
      else if (!strcmp(argv[z] + 1, "c"))
      {
        if (z >= argc - 1)
          pObjExtension = NULL;
        else
        {
          pObjExtension = argv[z + 1];
          used[z + 1] = 1;
        }
      }
      else if (!strcmp(argv[z] + 1, "r"))
      {
        ArgsAreObj = 1;
      }
      else if (argv[z][1] == 'I')
      {
        if (!argv[z][2])
        {
          if (z < argc - 1)
          {
            if (argv[z + 1][0] == '-')
              fprintf(stderr, "%s: taking '%s' as include path, is this correct?\n", argv[0], argv[z + 1]);
            AddStringListLast(&inc_path_list, argv[z + 1]);
            used[z + 1] = 1;
          }
        }
        else
          AddStringListLast(&inc_path_list, argv[z] + 2);
      }
      else if (argv[z][1] == 'D')
      {
        if (!argv[z][2])
        {
          if (z < argc - 1)
          {
            if (argv[z + 1][0] == '-')
              fprintf(stderr, "%s: taking '%s' as macro, is this correct?\n", argv[0], argv[z + 1]);
            AddStringListLast(&def_list, argv[z + 1]);
            used[z + 1] = 1;
          }
        }
        else
          AddStringListLast(&def_list, argv[z] + 2);
      }
      else
      {
        fprintf(stderr, "unknown option: %s\n", argv[z]);
        exit(2);
      }
    }

  if (pDepFileName)
  {
    pDepFile = fopen(pDepFileName, "w");
    if (!pDepFile)
    {
      perror(pDepFileName);
      exit(errno);
    }
  }
  else
    pDepFile = stdout;

  fprintf(pDepFile, "# auto-generated by %s - do not edit\n\n", *argv);

  InitStringList(&FileList);
  InitStringList(&SrcList);
  for (z = 1; z < argc; z++)
  {
    char SrcFileName[512];
    const char *pSrcFileName;

    if (used[z])
      continue;

    /* object files may be in paths different than root - strip path to deduce associated src file */

    if (ArgsAreObj)
    {
      char *pRun;

      for (pRun = argv[z] + strlen(argv[z]); pRun > argv[z]; pRun--)
        if ((*(pRun - 1) == '/') || (*(pRun - 1) == '\\'))
          break;
      /* leading h_ in object file name is used for host-side files */
      if (!strncmp(pRun, "h_", 2))
        pRun += 2;
      *SrcFileName = '\0'; strncat(SrcFileName, pRun, sizeof(SrcFileName) - 1);
      if (pObjExtension)
      {
        int l1 = strlen(SrcFileName), l2 = strlen(pObjExtension);

        if ((l1 > l2) && !memcmp(SrcFileName + l1 - l2, pObjExtension, l2))
          strcpy(SrcFileName + l1 - l2, ".c");
      }
      pSrcFileName = SrcFileName;
    }
    else
      pSrcFileName = argv[z];
    ClearStringList(&FileList);
    AddStringListLast(&SrcList, pSrcFileName);
    ParseFile(pSrcFileName, NULL, &FileList);

    if (!StringListEmpty(FileList))
    {
      fprintf(pDepFile, "%s: %s",
              pOutFileName ? pOutFileName : (ArgsAreObj ? argv[z] : getobj(argv[z])),
              pSrcFileName);
      while (True)
      {
        char *pIncFileName = MoveAndCutStringListFirst(&FileList);
        if (pIncFileName)
        {
          if (*pIncFileName)
            fprintf(pDepFile, " %s", pIncFileName);
          free(pIncFileName); pIncFileName = NULL;
        }
        else
          break;
      }
      fprintf(pDepFile, "\n\n");
    }
  }

  if (pDepFileName)
  {
    if (!StringListEmpty(SrcList))
    {
      fprintf(pDepFile, "%s:", pDepFileName);
      while (True)
      {
        char *pIncFileName = MoveAndCutStringListFirst(&SrcList);
        if (pIncFileName)
        {
          if (*pIncFileName)
            fprintf(pDepFile, " %s", pIncFileName);
          free(pIncFileName); pIncFileName = NULL;
        }
        else
          break;
      }
      fprintf(pDepFile, "\n");
    }
    fclose(pDepFile);
  }

  ClearStringList(&inc_path_list);

  return 0;
}
