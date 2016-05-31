%%   Matt McFarland
%
%   E91, lab 5, question 4
%

function [] = q4script()
    %% Define Constants and Functions
    % 
    % *Constants*
    
    start_t     = 1;
    end_t       = 2;
    start_y     = -1; 

    n_end       = 4;
    n           = 0:n_end;
    methods     = 5;
    
    %%
    % *Functions*
    
    Rate        = @(y,t)    1 ./ t^2 - y ./ t - y.^2;
    Y           = @(t)      -1 ./ t;         % Real Solution
    step_size   = @(n)      .2 .* (2.^-n);   % Step size (delta t)
    
    %%
    % *ODE Solution Functions*
    
    EulerFunc = @Euler;
    
    %% Create an error storage matrix for error of given method at t = 2 
    %
    %   Row signifies step size n (0, 1, 2, 3, 4...) 
    %   Column 1 = Y(t=2)  
    %   Column 2 = Eulers Method 
    %   Column 3 = Midpoint Method 
    %   Column 4 = Modified Euler's Method 
    %   Column 5 = 2-Step A-B/A-M Method (1 correction) 
    %   Column 6 = Runge-Kutta 4th Order 
    
    errors      = zeros(length(n),methods+1);
    errors(:,1) = Y(end_t);
    t = linspace(start_t,end_t,100); % Trace exact solutions for plotting
    y = Y(t);
    
    %% EULER'S METHOD
    
    %%
    % Create a matrix for holding values of t (n == row)
%     size = (end_t - start_t) / step_size(n(end));  
%     
%     Euler_t         = zeros(n(end)+1,size);
%     Euler_t(:,1)    = start_t;
%     
%     %%
%     % Create a matrix for holding values of y (n == row)   
%     Euler_y         = zeros(n(end)+1,size);
%     Euler_y(:,1)    = start_y;
%     
%     %%
%     % Create a matrix for holding errors values
%     Euler_error         = zeros(n(end)+1,size);
%     Euler_error(:,1)    = 0;
%     
%     %%
%     % Calculate Euler iterations for each n value
%     for i = 1:length(n)
%         delta_t = step_size(n(i));
%         steps = (end_t - start_t) / delta_t;
%         for j = 1:steps
%             Euler_t(i,j+1) = Euler_t(i,1) + j * delta_t;
%             Euler_y(i,j+1) = Euler(Rate, Euler_y(i,j), Euler_t(i,j), delta_t);
%             Euler_error(i,j+1) = abs(Y(Euler_t(i,j+1)) - Euler_y(i,j+1));
%         end
%     end
 
    %% Try out ODE Solver
    [Euler_t, Euler_y] = SolveODE(Rate, EulerFunc, step_size, ...
        start_y, start_t, end_t, n_end);
    
    %% Plot Euler Methods
    figure(1)
    hold on
    plot(t,y,'--');                % Real Solution
    
    legend_str1{1} = 'Exact';
    for i = 1:length(n)
        points = (end_t - start_t) / step_size(n(i)) + 1;
        plot(Euler_t(i,1:points) , Euler_y(i,1:points))
        legend_str1{i+1} = sprintf('Euler n = %d',n(i));
    end
    hold off
    legend(legend_str1,'Location','southeast');
    axis([start_t end_t -1 -.3])
    xlabel('t')
    ylabel('y')
    title_str = title('Euler Methods Approximations for stepsize $ \frac{1}{5*2^n} $');
    set(title_str,'Interpreter','Latex','FontSize',15)
    

    
    %% Plot Errors for Euler's N methods
%     figure(2)
%     hold on
%     for i = 1:length(n)
%         points = (end_t - start_t) / step_size(n(i)) + 1;
%         plot(Euler_t(i,1:points),log(Euler_error(i,1:points)),'x-')
%         % Lose first point at t = 0 because log (0) isn't displayed
%     end
%     hold off
%     legend(legend_str1{2:end},'Location','southeast');
%     xlabel('t')
%     ylabel('log |Y(t) - Y_n(t)|')
%     title('Log of Error Magnitude of Euler Approximation')

    %% Plot Midpoint Method Results
%     figure(3)
%     hold on
%     plot(t,y,'--');                % Real Solution
%     legend_str1{1} = 'Exact';
%     
%     for i = 1:length(n)
%         points = (end_t - start_t) / StepSize(n(i)) + 1;
%         plot(Mid_t(i,1:points) , Mid_y(i,1:points))
%         legend_str1{i+1} = sprintf('Midpoint n = %d',n(i));
%         
%         % Store error at (t_end) in errors matrix
%         errors(i,3) = abs(errors(i,1) - Mid_y(i,points));
%     end
%     hold off
%     legend(legend_str1,'Location','southeast');
%     xlabel('t')
%     ylabel('y')
%     title_str = title('Midpoint Methods Approximations for stepsize $ \frac{1}{5*2^n} $'); 
%     set(title_str,'Interpreter','Latex','FontSize',15)

end

%% Generic ODE Solution function
% Outputs a n X 2^n matrix of solution values
% where for values(row, column)
%   Row is the n in the step size calculation
%   Column is the ith step
% 
% Can be applied for 
function [t, y] = SolveODE(RateFunc,SolutionFunc,StepFunc,y_0,t_0,t_end,n_max)
    % --- INPUTS ---
    % RateFunc      = ODE to evaluate
    % SolutionFunc  = calculates w_i+1
    % StepFunc      = calculates $\Delta$t for given n
    % y_0           = initial y coordinate
    % t_0           = initial t coordinate
    % t_end         = final t coordinate to solve ODE at
    % n_max         = solve for n = 0, 1, 3 ... n_max
    
    % --- OUTPUTS ---
    % t             = (n_max + 1) X 2^n_max matrix with solutions t values
    % y             = (n_max + 1) X 2^n_max matrix with solutions y values
    
    % Find out how many steps will be needed for the n_max method
    max_len = (t_end - t_0) / StepFunc(n_max);
    t = zeros(n_max + 1, max_len);
    y = zeros(n_max + 1, max_len);
    
    t(:,1) = t_0;
    y(:,1) = y_0;
    
    n = 0:n_max;
    
    % Calculate Solutions iterations for each n value
    for i = 1:length(n)
        delta_t = StepFunc(n(i));
        steps = (t_end - t_0) / delta_t;
        for j = 1:steps
            t(i,j+1) = t(i,1) + j * delta_t;
            y(i,j+1) = SolutionFunc(RateFunc,y(i,j),t(i,j),delta_t);
        end
    end

end

%% Euler's Method for calculating the next y value for a given rate function, current (y,t), and step size
function [next_y] = Euler(rate_fun,y,t,step)
    next_y = rate_fun(y,t) * step + y;
end


%% Midpoint Method for solving ODE given rate function, current (y,t) and step size
function [next_y] = MidpointMethod(rate_fun,y,t,step)
    
end
