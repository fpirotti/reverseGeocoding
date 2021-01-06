#
# This is a Shiny web application. You can run the application by clicking
# the 'Run App' button above.
#
# Find out more about building applications with Shiny here:
#
#    http://shiny.rstudio.com/
#

library(shiny)
library(readr)
library(shinyalert)
library(shinyWidgets)
library(curl)      
library(shinyjs)  
library(httr)
library(openxlsx)
library(sf)
library(tidyverse)
# library(future)
# library(promises)
source("functions.R")

# plan(multisession)

# Define UI for application that draws a histogram
ui <- fluidPage(
    useShinyjs(),  # Include shinyjs
    tags$head(
      tags$style(HTML("
      @import url('//fonts.googleapis.com/css?family=Lobster|Cabin:400,700');
      .fontRed{
        color:red;
        font-weight:bold;
      }
      h4 {
        font-family: 'Lobster', cursive;
        font-weight: 500;
        line-height: 1.1;
        color: #48ca3b;
      }

    "))
    ),
    # Application title
    titlePanel("CIRGEO reverse geocoding"),

    # Sidebar with a slider input for number of bins 
    sidebarLayout(
        sidebarPanel(
            fileInput("file1", label = h3("File input")),
            fluidRow(
                column(6, checkboxInput("header", div(title="File con prima riga con intestazione (nome colonne)",
                                                      HTML("Header<sup>?</sup>")), TRUE) ) ,
                column(6, checkboxInput("forceNonCache", 
                                     div(title="Non utilizza la cache ma obbliga nuova interrogazione da Engine selezionata", 
                                         HTML("Force<sup>?</sup>")), F) )
            ),
            numericInput("maxErrs", div(title="Maximum number of allowed errors - leave 0 for infinite",
                                        HTML("Max Errors<sup>?</sup>") ), value = 0 ),
            shiny::actionButton("process", "START"),
            downloadButton('downloadData', 'Download XLS', icon=icon("table")),
            downloadButton('downloadData2', 'Download GPKG') 
            
        ),

        # Show a plot of the generated distribution
        mainPanel(
            fluidRow(column(12, shinyWidgets::prettyRadioButtons(
                "type", div(title="Una delle Engine di Geocoding - Att.ne Google a pagamento. 
NB: 'ALL' applica in preferenza BING, poi HERE e in ultima istanza Google.", 
                            HTML("ENGINE<sup>?</sup>"))
                ,inline = T
                ,list(
                    google = "G",
                    bing = "B",
                    here = "H",
                    ALL = "ALL"
                ), selected = "ALL"
            ))),
            progressBar(id = "progBar", value = 0, total = 5000, status = "warning"),
            
            h5("Preview loaded address") ,
            tableOutput("contents"),
            fluidRow(column(3, h5(HTML("Error Log Table: 
                                       tot errors <span id='tErrors'>0</span>") ) ), 
                     column(9, downloadButton("downloadErrLog", "Download Error Table") ) ),
            tableOutput("logTable")
        )
    )
)



# Define server logic required to draw a histogram
server <- function(input, output, session) {
    currentEngine<-"H"
    currentFile<-NULL
    currKey<-1
    mytable<-NULL
    mytable.output<-NULL
    stopIt<-F
    isRunning<-F
    
    rVals<-reactiveValues(errTable=data.frame(Engine=character(0),
                                              Error=character(0),
                                              Address=character(0)) 
                          )
    errLog<-tempfile("log")
    outFile<-paste(sep=".", tempfile("output"), "xlsx")
    outFileGeo<-paste(sep=".", tempfile("output"), "gpkg")
     
    observeEvent(rVals$errTable, {
      req(rVals$errTable)
      write_tsv(rVals$errTable,errLog)
    })
    
    output$logTable <- renderTable({
      req(rVals$errTable)
      rVals$errTable
    })
    
    # output$contents <- renderTable({ 
    #     req(rVals$outFile)
    #     rVals$outFile
    # })
     
    output$contents <- renderTable({ 
        file <- input$file1
        req(file)
        ext <- tools::file_ext(file$datapath) 
        
        if(file.size(file$datapath)/1000000 > 10){
            shinyWidgets::show_alert("File size is too big, please limit to 10 MB")
            return(NULL)
        }
        
        if( tolower(ext)=="zip"){
            cf<-unzip(file$datapath, list  = T)
            currentFile<<-list(name=cf[["Name"]], 
                               size=cf[["Length"]])
            unzip(file$datapath, overwrite = T)
        }
        else currentFile<<- file
        
        validate(need( is.element(tolower(ext),c("csv","zip", "txt","dat")), 
                                  "Please upload a file zip or CSV or TXT extensions. Must be a table"))
        
        mytable<<-tryCatch({ 
            read_tsv(file$datapath)
        }, error=function(e){
            return(e)
        })
        
        if(ncol(mytable)!=1){
            shinyWidgets::show_alert("Only one column with addresses please.")
            return(NULL)
        }
        
        # server
        updateProgressBar(session = session, id = "progBar", value = 0, total = nrow(mytable))
        mytable[1:4,]
    })
    
    ### DOWNLOAD BUTTONS --------
    output$downloadData <- downloadHandler(
 
      filename = function() {
        if(is.null(currentFile$name) || is.null(mytable.output)){
          shinyWidgets::show_alert("Non è stato ancora elaborato alcun dato")
          return(NULL)
        }
        bn<-basename(currentFile$name)
        raster::extension(bn)<-""
        paste(bn, '_', isolate(input$type), '_',
              format(Sys.Date(), "%Y%m%dT%I%M%S"), '.xlsx', sep='')
      },
      content = function(con) {
           openxlsx::write.xlsx(mytable.output, con)
      }
    )
    
    output$downloadData2 <- downloadHandler(

      filename = function() {
        if(is.null(currentFile$name) || is.null(mytable.output)){
          shinyWidgets::show_alert("Non è stato ancora elaborato alcun dato")
          return(NULL)
        }
        bn<-basename(currentFile$name)
        raster::extension(bn)<-""
        paste(bn, '_', isolate(input$type), '_',
              format(Sys.Date(), "%Y%m%dT%I%M%S"), '.gpkg', sep='')
      },
      content = function(con) {  
        sfobj<-st_as_sf(mytable.output, coords=c("long", "lat") )
        st_crs(sfobj) = 4326
        sf::write_sf(sfobj, con)
      }
    )
    
    output$downloadErrLog <- downloadHandler(
      filename = function() {
        if(is.null(currentFile$name) || is.null(mytable.output)){
          shinyWidgets::show_alert("Non è stato ancora elaborato alcun dato")
          return(NULL)
        }
        bn<-basename(currentFile$name)
        raster::extension(bn)<-""
        paste(bn, '_', isolate(input$type), '_',
              format(Sys.Date(), "%Y%m%dT%I%M%S_ErrorTable"), '.xlsx', sep='')
      },
      content = function(con) {
        openxlsx::write.xlsx(isolate(rVals$errTable), con)
      }
    )
    
 
    
    observeEvent(input$type, {
      if(input$type=="ALL"){
        currentEngine<<-"H"
      } else {
        currentEngine<<-input$type
      }
      print(currentEngine)
    })
    
    
    observeEvent(input$process, {
        req(mytable) 
      
        if(isRunning){
          shinyjs::html("process", HTML("<div onclick='Shiny.setInputValue(\"shouldStop\", true, {priority: \"event\"})'>RUNNING!</div>") )
          shinyWidgets::show_alert("Process already running")
          return(NULL)
        }   
      
        isRunning<-T
      
        resultTable<-list()
 
        ## each row in table
        ttrows<-nrow(mytable)
        mytable.output<<-NULL
        totErrs<-0
        
        shinyjs::html("process", HTML("<div onclick='Shiny.setInputValue(\"shouldStop\", true, {priority: \"event\"})'>RUNNING!</div>") )
 
        #future({  --- check http://blog.fellstat.com/?p=407
        
          for(i in 1:ttrows){
            address<-mytable[i,1][[1]] 
            if(is.null(cacheFile[[currentEngine]][[address]]) || isolate(input$forceNonCache) ){

                pC<- parseContent(isolate(input$type), address)
              
                if(is.list(pC)) {
                    resultTable[[address]]<-pC
                } else {
                    totErrs<-totErrs+1
                    shinyjs::html("tErrors", as.character(totErrs))
                    shinyjs::addClass("tErrors", "fontRed")
                    
                    isolate(rVals$errTable) %>% add_row(Engine=isolate(input$type) ,
                                                        Error=as.character(totErrs) ,
                                                        Address=address ) 
                                                   
                    if(isolate(input$maxErrs) > 0 && totErrs>isolate(input$maxErrs)){
                        shinyWidgets::show_alert(sprintf("Too many errors, aborting. 
                                                 Last error is: '%s'", pC))
                        saveRDS(cacheFile, "cacheFile.rds")
                        return(NULL)
                    }
                    
                    resultTable[[address]]<- list(
                        lat=0,
                        long=0,
                        accur=pC,
                        formattedAddress="NA",
                        type = currentEngine
                    )
                }
                
                updateProgressBar(session = session, id = "progBar", value = i, total = ttrows )   
                
            } else { 
                resultTable[[i]] <- cacheFile[[currentEngine]][[address]]
            } 

        
            if(i%%as.integer(ttrows/200)==0){ 
              updateProgressBar(session = session, id = "progBar", value = i, total = ttrows )
               
            }
          }

          saveRDS(cacheFile, "cacheFile.rds")
          tt <- data.table::rbindlist(resultTable )
          
          tt$Original_Address<-mytable[,1]
          
          print(length(unique(unlist(mytable[,1]))))
          
          mytable.output<<-tt
          isRunning<-F
          updateProgressBar(session = session, id = "progBar", value = ttrows, total = ttrows )   
          shinyjs::html("process", "START")
          
      #  })

          bn<-basename(currentFile$name)
          raster::extension(bn)<-""
          con<-paste(bn, '_', isolate(input$type), '_',
                format(Sys.Date(), "%Y%m%dT%I%M%S"), '.xlsx', sep='')
          
          con2<-paste(bn, '_', isolate(input$type), '_',
                     format(Sys.Date(), "%Y%m%dT%I%M%S"), '.gpkg', sep='')
          
          outfileTot<-list(geo=tt, 
               errors=isolate(rVals$errTable) )
          
          saveRDS(outfileTot, "outfileTot.rds")
          openxlsx::write.xlsx(outfileTot, con) 
           
          
          sfobj<-st_as_sf(mytable.output, coords=c("long", "lat") )
          st_crs(sfobj) = 4326
          sf::write_sf(sfobj, con2)
        
    })
}

# Run the application 
shinyApp(ui = ui, server = server)
