function os = cns_osname

if ~isempty(strmatch('GLN', computer))
    os = 'LINUX';
elseif ~isempty(strmatch('PC', computer))
    os = 'WIN';
elseif ~isempty(strmatch('MAC', computer))
    os = 'MAC';
elseif ~isempty(strmatch('SOL', computer))
    os = 'SOL';
else
    error('unknown architecture');
end

return;