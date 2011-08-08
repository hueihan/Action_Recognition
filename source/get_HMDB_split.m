

function [train_fnames,test_fnames]= get_HMDB_split(isplit,splitdir)
saction =      {'brush_hair','cartwheel','catch','chew','clap','climb','climb_stairs',...
      'dive','draw_sword','dribble','drink','eat','fall_floor','fencing',...
      'flic_flac','golf','handstand','hit','hug','jump','kick_ball',...
      'kick','kiss','laugh','pick','pour','pullup','punch',...
      'push','pushup','ride_bike','ride_horse','run','shake_hands','shoot_ball',...
      'shoot_bow','shoot_gun','sit','situp','smile','smoke','somersault',...
      'stand','swing_baseball','sword_exercise','sword','talk','throw','turn',...
      'walk','wave'};


       for iaction = 1:length(saction)
	 itr = 1;
	 ite = 1;
	 fname = sprintf('%s/%s_test_split%d.txt',splitdir,saction{iaction},isplit);

	 fid = fopen(fname);
	 
	 while 1
	   tline = fgetl(fid);
	   if tline==-1
	     break
	   end
	   [tline, u] = strtok(tline,' ');   
	   u = str2num(u);
	   
	   video = sprintf('%s.avi',tline(1:end-4));
    
	   if u==1 % ignore testing
	       train_fnames{iaction}{itr} = tline;
	       itr = itr + 1;
	   elseif u==2
	       test_fnames{iaction}{ite} = tline;
	       ite = ite + 1;
	   end
	 end
	 fclose(fid);
       end
       
