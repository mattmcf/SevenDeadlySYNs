function [ Area ] = AQ(func,left_x, right_x, error)
    %% Recursive Adaptive Quadrature Function
    % Applies the adaptive quadrature recursive strategy until 
    % the error threshold is reached
    
    %%
    % Use a conservative error reduction of a factor of 10
    threshold = 10;
    
    %%
    % Calculate midpoint and h
    midpoint    = (left_x + right_x)/2;
    h1          = (right_x - left_x)/2;
    
    %%
    % Get function evaluations at left, right and mid points in bound
    f_left      = func(left_x);
    f_right     = func(right_x);
    f_mid       = func(midpoint);     
    
    %%
    % Calculate quadrature for total, left and right segments
    %
    % Use Simpson Rule
    
    Total_Area = h1/3 * (f_left + 4*f_mid + f_right);
    
    %% 
    % Split left and right halves and apply Simpson's Rule on those segments
    
    h2 = h1 / 2;
    
    x_quarter   = (left_x + midpoint) / 2;
    x_3quarter  = (midpoint + right_x) / 2;
    
    f_quarter   = func(x_quarter);
    f_3quarter  = func(x_3quarter);
    
    Left_Area   = h2/3 * (f_left + 4*f_quarter + f_mid);
    Right_Area  = h2/3 * (f_mid + 4*f_3quarter + f_right);
    
    %%
    % Evaluate if error threshold has been reached
    %
    % If it hasn't, apply Adaptive Quadrature on each segment half
    
    if ( (Total_Area - Left_Area - Right_Area) < (threshold*error) )
        Area = Left_Area + Right_Area;
    else
        Area = AQ(func,left_x,midpoint,error/2) + ...
            AQ(func,midpoint,right_x,error/2);
    end

end



