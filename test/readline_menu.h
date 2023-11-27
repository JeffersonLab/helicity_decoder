/* readline_menu.h - menu system routines using readline library. */

/*
 * This software uses the GNU Readline library, version 8-2, licensed
 * under the GNU General Public License, version 3.  See
 * https://www.gnu.org/software/readline/index.html for more
 * information.
 *
 * This software also uses code from the following example in the GNU
 * Readline library:
 * https://tiswww.cwru.edu/php/chet/readline/readline.html#A-Short-Completion-Example
 *
 */

#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>

/* A structure which contains information on the commands this program
   can understand. */

/* Forward declarations. */
char *stripwhite();
COMMAND *find_command();

/* The name of this program, as taken from argv[0]. */
char *progname;

/* When non-zero, this global means the user is done using this program. */
int done;

char *
dupstr(char *s)
{
  char *r;

  r = malloc(strlen(s) + 1);
  strcpy(r, s);
  return (r);
}

/* Execute a command line. */
int
execute_line(char *line)
{
  register int i;
  COMMAND *command;
  char *word;

  /* Isolate the command word. */
  i = 0;
  while(line[i] && whitespace(line[i]))
    i++;
  word = line + i;

  while(line[i] && !whitespace(line[i]))
    i++;

  if(line[i])
    line[i++] = '\0';

  command = find_command(word);

  if(!command)
    {
      fprintf(stderr, "%s: No such command\n", word);
      return (-1);
    }

  /* Get argument to command, if any. */
  while(whitespace(line[i]))
    i++;

  word = line + i;

  /* Call the function. */
  return ((*(command->func)) (word));
}

/* Look up NAME as the name of a command, and return a pointer to that
   command.  Return a NULL pointer if NAME isn't a command name. */
COMMAND *
find_command(char *name)
{
  register int i;

  for(i = 0; commands[i].name; i++)
    if(strcmp(name, commands[i].name) == 0)
      return (&commands[i]);

  return ((COMMAND *) NULL);
}

/* Strip whitespace from the start and end of STRING.  Return a pointer
   into STRING. */
char *
stripwhite(char *string)
{
  register char *s, *t;

  for(s = string; whitespace(*s); s++)
    ;

  if(*s == 0)
    return (s);

  t = s + strlen(s) - 1;
  while(t > s && whitespace(*t))
    t--;
  *++t = '\0';

  return s;
}

/* **************************************************************** */
/*                                                                  */
/*                  Interface to Readline Completion                */
/*                                                                  */
/* **************************************************************** */

char *command_generator PARAMS((const char *, int));
char **fileman_completion PARAMS((const char *, int, int));

/* Tell the GNU Readline library how to complete.  We want to try to complete
   on command names if this is the first word in the line, or on filenames
   if not. */
void
initialize_readline(char *name)
{
  /* Allow conditional parsing of the ~/.inputrc file. */
  if(name)
    rl_readline_name = dupstr(name);
  else
   rl_readline_name = "readline";

  /* Tell the completer that we want a crack first. */
  rl_attempted_completion_function = fileman_completion;
}

/* Attempt to complete on the contents of TEXT.  START and END bound the
   region of rl_line_buffer that contains the word to complete.  TEXT is
   the word to complete.  We can use the entire contents of rl_line_buffer
   in case we want to do some simple parsing.  Return the array of matches,
   or NULL if there aren't any. */
char **
fileman_completion(const char *text, int start, int end)
{
  char **matches;

  matches = (char **) NULL;

  /* If this word is at the start of the line, then it is a command
     to complete.  Otherwise it is the name of a file in the current
     directory. */
  if(start == 0)
    matches = rl_completion_matches(text, command_generator);

  return (matches);
}

/* Generator function for command completion.  STATE lets us know whether
   to start from scratch; without any state (i.e. STATE == 0), then we
   start at the top of the list. */
char *
command_generator(const char *text, int state)
{
  static int list_index, len;
  char *name;

  /* If this is a new word to complete, initialize now.  This includes
     saving the length of TEXT for efficiency, and initializing the index
     variable to 0. */
  if(!state)
    {
      list_index = 0;
      len = strlen(text);
    }

  /* Return the next name which partially matches from the command list. */
  while((name = commands[list_index].name) != NULL)
    {
      list_index++;

      if(strncmp(name, text, len) == 0)
	return (dupstr(name));
    }

  /* If no names matched, then return NULL. */
  return ((char *) NULL);
}


/* Print out help for ARG, or for all of the commands if ARG is
   not present. */
int
com_help(char *arg)
{
  register int i;
  int printed = 0;

  printf("\n");
  for(i = 0; commands[i].name; i++)
    {
      if(!*arg || (strcmp(arg, commands[i].name) == 0))
	{
	  printf("%15s   %s.\n", commands[i].name, commands[i].doc);
	  printed++;
	}
    }

  if(!printed)
    {
      printf("No commands match `%s'.  Possibilties are:\n", arg);

      for(i = 0; commands[i].name; i++)
	{
	  /* Print in six columns. */
	  if(printed == 6)
	    {
	      printed = 0;
	      printf("\n");
	    }

	  printf("%s\t", commands[i].name);
	  printed++;
	}

      if(printed)
	printf("\n");
    }

  printf("\n");
  return (0);
}

/* The user wishes to quit using this program.  Just set DONE non-zero. */
int
com_quit(char *arg)
{
  done = 1;
  return (0);
}

void
readline_menu_loop(char *prompt)
{
  char *line, *s;

  /* Loop reading and executing lines until the user quits. */
  for(; done == 0;)
    {
      line = readline(prompt);

      if(!line)
	break;

      /* Remove leading and trailing whitespace from the line.
         Then, if there is anything left, add it to the history list
         and execute it. */
      s = stripwhite(line);

      if(*s)
	{
	  add_history(s);
	  execute_line(s);
	}

      free(line);
    }

}
