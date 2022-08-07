#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define STDIN       0
#define STDOUT      1
#define SERIAL      1
#define PARALLEL    2
#define NONE       -10
#define WRITE       10
#define APND        20
#define READ        30
#define PIPE        40
#define ARGS        10
#define SIZE        128

struct comm_info {
  char* args_buff[ ARGS ];
  char* args_temp[ ARGS ];
  char rdir_path[ SIZE ];
  int rdir_flag;
  int args_count;
  int commands_count;
};

struct comm_prsd {
  char* arguments[ ARGS ];
  int arguments_count;
  int input_from;
  int output_to;
  int exe_mode;
};

int stout; // STDIN descriptor
int stin; // STDOUT descriptor
int some_weird_value[ 2 ]; // pipe descriptors
struct comm_info command; // General command information
struct comm_prsd commands[ ARGS ]; // Information about each executable


// Executes commands. Not people.
void executor( int cmd_n, int* pp ) {
  int pid;
  if( commands[ cmd_n ].exe_mode == SERIAL ) {
    if( fork( ) == 0 ) {
      if( command.rdir_flag == READ && cmd_n == 0 ) {
        close( STDIN );
        open( command.rdir_path, O_RDONLY ); }
      if( command.rdir_flag == WRITE ) {
        close( STDOUT );
        open( command.rdir_path, O_WRONLY | O_CREATE | O_TRUNC ); }
      if( command.rdir_flag == APND ) {
        close( STDOUT );
        open( command.rdir_path, O_WRONLY | O_CREATE ); }
      if( exec( commands[ cmd_n].arguments[ 0 ], commands[ cmd_n ].arguments ) == - 1) {
        printf("Unknown command %s\n", commands[ cmd_n ].arguments[ 0 ]);
        exit( - 1 );
      }
    }
    else {
      pid = wait( 0 );
      kill( pid );
      return;
    }
  }
  else if( commands[ cmd_n ].exe_mode == PARALLEL ) {
    if( commands[ cmd_n ].output_to == PIPE ) {
      int p[ 2 ];
      pipe( p );
      if( fork( ) == 0 ) {
        if( commands[ cmd_n ].input_from == PIPE ) {
          close( 0 );
          dup( pp[ 0 ] );
          close( pp[ 0 ] );
        }
        else if( commands[ 0 ].input_from == READ && cmd_n == 0 ) {
          close( 0 );
          open( command.rdir_path, O_RDONLY );
        }
        close( 1 );
        dup( p[ 1 ] );
        close( p[ 1 ] );
        if( exec( commands[ cmd_n ].arguments[ 0 ], commands[ cmd_n ].arguments ) == - 1 ) {
          printf( "Unknown command %s\n", commands[ cmd_n ].arguments[ 0 ]);
          exit( - 1 );
        }
      }
      else executor( ++cmd_n, p );
    }
    else {
      if( fork( ) == 0 ) {
          close( 0 );
          dup( pp[ 0 ]);
          close( pp[ 0 ]);
          if( commands[ cmd_n ].output_to == NONE ) {
            close( STDOUT );
            dup( stout );
            if ( exec( commands[ cmd_n ].arguments[ 0 ], commands[ cmd_n ].arguments ) == - 1 ) {
              printf( "Unknown command %s\n", commands[ cmd_n ].arguments[ 0 ]);
              exit( - 1 );
            }
          }
        if( commands[ cmd_n ].output_to == WRITE ) {
          close( STDOUT );
          open( command.rdir_path, O_WRONLY | O_CREATE | O_TRUNC );
          if( exec( commands[ cmd_n ].arguments[ 0 ], commands[ cmd_n ].arguments ) == - 1 ) {
            printf( "Unknown command %s\n", commands[ cmd_n ].arguments[ 0 ]);
            exit( - 1 );
          }
        }
        if( commands[ cmd_n ].output_to == APND ) {
          close( STDOUT );
          open( command.rdir_path, O_WRONLY | O_CREATE );
          if( exec( commands[ cmd_n ].arguments[ 0 ], commands[ cmd_n ].arguments ) == - 1 ) {
            printf( "Unknown command %s\n", commands[ cmd_n ].arguments[ 0 ]);
            exit( - 1 );
          }
        }
      }
      else {
        pid = wait( 0 );
        kill( pid );
        return;
      }
    }
  }
}


// Fills up the command structure and the structures in commands array.
int comm_parser( void ) {

  int breaker = 0, repeater = 0;
  if( command.args_temp[ 0 ] != 0 ) command.commands_count = 1;
  else return - 1;

  for( int i = 0; i < command.args_count; i++ ) {

    if( repeater == 1 ) {
      strcpy( command.rdir_path, command.args_buff[ i ] );
      repeater = 0;
      continue;
    }

    if( strcmp( command.args_buff[ i ], "|" ) == 0 ) {
      commands[ command.commands_count - 1 ].output_to = PIPE;
      commands[ command.commands_count - 1 ].exe_mode = PARALLEL;
      commands[ command.commands_count ].input_from = PIPE;
      commands[ command.commands_count ].exe_mode = PARALLEL;
      command.commands_count++;
      breaker = 0;
      continue;
    }

    if( strcmp( command.args_buff[ i ], ">" ) == 0 ) {
        command.rdir_flag = WRITE;
        repeater = 1;
        continue;
    }

    if( strcmp( command.args_buff[ i ], ">>" ) == 0 ) {
        command.rdir_flag = APND;
        repeater = 1;
        continue;
    }

    if( strcmp( command.args_buff[ i ], "<" ) == 0 ) {
        command.rdir_flag = READ;
        repeater = 1;
        continue;
    }

    else {
      commands[ command.commands_count - 1 ].arguments[ breaker ] = command.args_buff[ i ];
      commands[ command.commands_count - 1 ].arguments_count++;
      ++breaker;
    }
  }

  if( command.rdir_flag == WRITE  ) commands[ command.commands_count - 1 ].output_to = WRITE;
  else if( command.rdir_flag == APND  ) commands[ command.commands_count - 1 ].output_to= APND;
  else if( command.rdir_flag == READ  ) commands[ 0 ].input_from = READ;
  return 0;
}


// Allocates the necessary memory for string arrays when the shell is launched for the first time.
void allocate_memory( void ) {

  for( int i = 0; i < ARGS; i++ ) {
    command.args_buff[ i ] = ( char* )malloc( SIZE );
    command.args_temp[ i ] = ( char* )malloc( SIZE );

    for( int j = 0; j < ARGS; j++ ) commands[ i ].arguments[ j ] = ( char* )( malloc( SIZE ) );
  }
}


// Cleans up command arrays and structures.
void comm_reset( void ) {

  command.args_count = 0;
  command.rdir_flag = NONE;
  memset( command.rdir_path, 0, SIZE );

  for( int i = 0; i < ARGS; i++ ) {
     memset( command.args_buff[ i ], 0, SIZE );
     command.args_temp[ i ] = 0;
     commands[ i ].input_from = NONE;
     commands[ i ].output_to = NONE;
     commands[ i ].exe_mode = SERIAL;
     commands[ i ].arguments_count = 0;

     for( int j = 0; j < ARGS; j++ ) commands[ i ].arguments[ j ] = 0;
   }
 }


// Splits strings, fills up some command buffers and manages list commands if these are present.
void inp_parser( char* in ) {

  int j = 0, x = 0;
  for( int i = 0; i < strlen( in ); i++ ) {

    if( ( in[ i ] == ' ' || in[ i ] == '\n' ) && j == 0 && x == 0 ) continue;

    else if( in[ i ] == ' ' || in[ i ] == '\n' ) {

      if( in[ i ] == '\n' && i > 0) {
        command.args_buff[ j++ ][ x ] = '\0';
        command.args_temp[ j - 1 ] =  command.args_buff[ j - 1 ];
        command.args_count = j;
        j = 0;
        x = 0;

        if( command.args_count > 0 ) {
          comm_parser( );
          executor( 0, some_weird_value );
        }
          comm_reset( );
          break;
      }
      command.args_buff[ j++ ][ x ] = '\0';
      command.args_temp[ j - 1 ] =  command.args_buff[ j - 1 ];
      if( in[ 0 ] == '\n' ) j--;
      x = 0;
      continue;
    }

    else if( in[ i ] == '>') {

      if( in[ i - 1 ] != ' ' && in[ i - 1 ] != '>' ) {
        command.args_buff[ j++ ][ x ] = '\0';
        command.args_temp[ j - 1 ] = command.args_buff[ j - 1 ];
        command.args_buff[ j ][ 0 ] = in [ i ];
        x = 0;
      }

      if( in[ i + 1 ] != ' ' && in[ i + 1 ] != '>' ) {
        command.args_buff[ j ][ x ] = in[ i ];
        command.args_buff[ j++ ][ ++x ] = '\0';
        command.args_temp[ j - 1 ] = command.args_buff[ j - 1 ];
        x = 0;
      }

      else if( in[ i + 1] == '>' ) {
        command.args_buff[ j ][ x++ ] = in[ i ];
        continue;
      }
      else command.args_buff[ j ][ x++ ] = in[ i ];
    }

    else if( in[ i ] == '<' ) {

      if( in[ i - 1] != ' ') {
        command.args_buff[ j++ ][ x ] = '\0';
        command.args_temp[ j - 1 ] = command.args_buff[ j - 1 ];
        command.args_buff[ j ][ 0 ] = in[ i ];
        x = 0;
      }

      if( in[ i + 1 ] != ' ') {
        command.args_buff[ j ][ 0 ] = in[ i ];
        command.args_buff[ j++ ][ 1 ] = '\0';
        x = 0;
      }
      else command.args_buff[ j ][ x++ ] = in[ i ];
    }

    else if( in[ i ] == '|' ) {

      if( in[ i - 1] != ' ') {
        command.args_buff[ j++ ][ x ] = '\0';
        command.args_temp[ j - 1 ] = command.args_buff[ j - 1 ];
        command.args_buff[ j ][ 0 ] = in[ i ];
        x = 0;
      }
      if( in[ i + 1 ] != ' ') {
        command.args_buff[ j ][ 0 ] = in[ i ];
        command.args_buff[ j++ ][ 1 ] = '\0';
        x = 0;
      }
      else command.args_buff[ j ][ x++ ] = in[ i ];
    }

    else if( in[ i ] == ';' ) {

      if( in[ i - 1 ] == ' ' ) {
        command.args_count = j;
        j = 0;
        x = 0;

        if( command.args_count > 0 ) {
          comm_parser( );
          executor( 0, some_weird_value );
        }

        comm_reset( );
        if( in[ i + 1 ] == '\0') break;
        continue;
      }

      else {

        command.args_buff[ j++ ][ x ] = '\0';
        command.args_temp[ j - 1 ] = command.args_buff[ j - 1 ];
        command.args_count = j;
        j = 0;
        x = 0;

        if( command.args_count > 0 ) {
          comm_parser( );
          executor( 0, some_weird_value );
        }

        comm_reset( );
        if( in[ i + 1 ] == '\0') break;
        continue;
      }
    }

    else if( in[ i ] == '\0' ) {

      command.args_count = ++j;
      if( command.args_count > 0 ) {
        comm_parser( );
        executor( 0, some_weird_value );
      }
      comm_reset( );
      break;
    }
    else command.args_buff[ j ][ x++ ] = in[ i ];
  }
  return;
}


// Reads input and manages parsting and execution. Returns -1 if the shell is to be exited.
int new_comm( char* buffer, int bytesize ) {

  memset( buffer, 0, bytesize );
  gets( buffer, bytesize );

  if( buffer[ 0 ] == 'c' && buffer[ 1 ] == 'd' && buffer[ 2 ] == ' ' ) {
    buffer[ strlen( buffer ) - 1 ] = 0;

    if( chdir( buffer + 3 ) == -1 ) {
      printf( "Unknown directory %s", buffer + 3 );
      return 0;
    }

    else {
      printf( ">>>" );
      return 0;
    }
  }
  if( buffer[ 0 ] == 'e' && buffer[ 1 ] == 'x' && buffer[ 2 ] == 'i' && buffer[ 3 ] == 't' && strlen( buffer ) == 5 ) return - 1;

  close( STDOUT );
  dup( stout );

  close( STDIN );
  dup( stin );

  inp_parser( buffer );
  printf( ">>>" );

  return 0; }


// Keeps looping indefinitely until the shell is exited.
int main( void ) {
  
  char user_input[ 1280 ];
  stin = ( dup ( STDIN ) );
  stout = ( dup( STDOUT ) );

  allocate_memory( );
  comm_reset( );
  printf( ">>>" );

  while( new_comm( user_input, sizeof( user_input ) ) >= 0 ) {}

  free( command.args_buff );
  free( command.args_temp );
  for( int i = 0; i < ARGS; i++ ) free( commands[ i ].arguments );
  exit( 0 );
}
