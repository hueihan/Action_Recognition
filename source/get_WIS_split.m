function [train_fnames,test_fnames] = get_WIS_split(isplit,splitdir)
    saction = {'bend','jack','jump','pjump','run','side','skip','walk','wave1','wave2'};
    sactor = {'daria','denis','eli','ido','ira','lena','lyova','moshe','shahar'};
    test_actor_id = isplit;
    train_actor_id = setdiff(1:length(sactor),isplit);

    for isa = 1:length(saction)
        for it = 1:length(test_actor_id);
            test_fnames{isa}{it} = sprintf('%s_%s.avi',sactor{it},saction{isa});
        end
    end

    for isa = 1:length(saction)
        for it = 1:length(train_actor_id);
            train_fnames{isa}{it} = sprintf('%s_%s.avi',sactor{it},saction{isa});
        end
    end

