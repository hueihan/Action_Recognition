function msg = cns_error

err = lasterror;

lines = strread(err.message, '%s', 'delimiter', sprintf('\n'));

if (numel(lines) < 2) || isempty(strmatch('Error using ==> ', lines{1}))

    msg = err.message;

else

    msg = sprintf('%s\n', lines{2 : end});
    msg(end) = '';

end

return;
